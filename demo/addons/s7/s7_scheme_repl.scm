;;; s7_scheme_repl_server.scm
;;;
;;; Compiles geiser-godot-s7 requests into Scheme
;;; strings that once evaluated (via eval-string)
;;; will produce the expected geiser response.
;;;
;;; This is so the repl environment doesn't leak
;;; into the target environment for evaluation and
;;; it is used by SchemeReplServer to target separate
;;; Scheme nodes without cross contamination.
;;;
;;; The entry point is compile-geiser-request.
;;;
;;; See test-main.scm for examples of exchanges between
;;; geiser and the server.

;; Copyright (C) 2024 Rodrigo B. de Oliveira
;; Author: Rodrigo B. de Oliveira (rbo@acm.org)
;; Maintainer: Rodrigo B. de Oliveira (rbo@acm.org)
;; Keywords: languages, godot, s7, scheme, geiser
;; Homepage: https://github.com/bamboo/godot-s7-scheme
;; SPDX-License-Identifier: BSD-3-Clause
;; Version: 0.1.0

(define (compile-eval-request-string code-string f)
  (object->string
   (let ((r (gensym))
         (o (gensym)))
     `(let* ((,r #<unspecified>)
             (,o (with-output-to-string
                   (lambda ()
                     (set! ,r (eval-string ,code-string (rootlet)))))))
        ,(f r o)))))

(define (geiser-eval-format result output)
  `(object->string
    `((result ,(object->string ,result))
      (output . ,,output))))

(define (simple-eval-format result output)
  `(let ((result-str (object->string ,result)))
     (if (= 0 (string-length ,output))
         result-str
         (string-append ,output "\n" result-str))))

(define (compile-eval-request code)
  (compile-eval-request-string (object->string code) geiser-eval-format))

(define (compile-simple-repl-request code-string)
  (compile-eval-request-string code-string simple-eval-format))

(define (compile-completions-request code)
  "Handles ge:completions"
  ;; using single character names to avoid polutting
  ;; the symbol table that might be used for completion
  (let* ((p (car code))
         (l (string-length p)))

    (compile-eval-request
     `(let ((r '()))

        (for-each
         (lambda (s)
           (let ((n (symbol->string s)))
             (when (and (>= (string-length n) ,l)
                        (string=? ,p (substring n 0 ,l))
                        (defined? s))
               (set! r (cons n r)))))
         (symbol-table))
        r))))

(define (compile-symbol-documentation-request code)
  "Handles ge:symbol-documentation"
  (let ((s (cadar code)))
    (compile-eval-request
     `(let ((s (quote ,s)))
        (cond
         ((defined? s)
          `(("signature" . ,s)
            ("docstring" . ,(documentation s))))
         (#t #f))))))

(define (compile-autodoc-request code)
  (compile-eval-request
   (let ((s (caadar code)))
     (cond
      ((symbol? s)
       `(let ((s (quote ,s)))
          (cond
           ((defined? (quote ,s))
            (let ((v ,s))
              (cond
               ((or (procedure? v) (syntax? v))
                `((,s ("args" (("required" ...) ("optional") ("key"))))))
               (#t
                `((,s ("args") ("value" . ,(object->string v))))))))
           (#t (list)))))
      (#t (list))))))

(define (empty-response)
  (object->string
   (geiser-eval-format '() "")))

(define (compile-geiser-command-request command-string)
  (call-with-input-string command-string
    (lambda (p)
      (case (read p)
        ((geiser-eval)
         (case (read p)
           ((#f)
            (let ((code (read p)))
              (case code
                ((ge:autodoc) (compile-autodoc-request (read p)))
                ((ge:symbol-documentation) (compile-symbol-documentation-request (read p)))
                ((ge:completions
                  ge:module-completions) (compile-completions-request (read p)))

                ((ge:add-to-load-path
                  ge:symbol-location
                  ge:module-location) (empty-response))

                (else (compile-eval-request code)))))))
        (else (empty-response))))))

(define (compile-geiser-request request-string)
  "Compiles a geiser request into an expression that can be evaluated to produce the
corresponding geiser response."
  ;; geiser commands start with a comma,
  ;; otherwise it's a simple repl interaction
  (if (char=? #\, (request-string 0))
      (compile-geiser-command-request (substring request-string 1))
      (compile-simple-repl-request request-string)))
