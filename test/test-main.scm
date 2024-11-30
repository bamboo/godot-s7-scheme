(define-macro (assert-equals l r)
  `(let ((l' ,l)
         (r' ,r))
     (when (not (equal? l' r'))
       (let ((lq (quote ,l))
             (rq (quote ,r)))
         (error 'assertion-error "ERROR~%# (assert-equals ~a ~a)~%## ~a~%`~a`~%## ~a~%`~a`" lq rq lq l' rq r')))))

(define (godot scene)
  (let* ((program (or (getenv "GODOT") "godot"))
         (cmd (format #f "~a --path demo ~a --headless --quit" program scene)))
    (format #t "~a~%" cmd)
    (system cmd #t)))

(define (read-file path)
  (call-with-input-file path
    (lambda (p) (read-string 65535 p))))

(define* (golden-scene-test scene golden-file)
  (let ((output-file "bin/s7_scheme_tests.txt"))
    (call-with-output-file output-file
      (lambda (p) (write-string (godot scene) p)))
    (unless (= 0 (system (format #f "diff --color=auto -u ~a ~a" golden-file output-file)))
      (error 'assertion-error "ERROR: output of scene `~a` doesn't match golden file `~a`." scene golden-file))))

(define (golden-scene-tests)
  (golden-scene-test
   :scene "addons/s7/test/s7_scheme_tests.tscn"
   :golden-file "test/golden/s7_scheme_tests.txt"))

(define (eval-geiser-request r)
  (eval-string (compile-geiser-request r) (rootlet)))

(define (assert-geiser-request-string request expected)
  (assert-equals
   (eval-geiser-request request)
   expected))

(define (assert-geiser-request request expected)
  (assert-geiser-request-string request (object->string expected)))

(define completion-candidate-1 #f)

(define (completion-candidate-2) #f)

(define autodoc-variable 42)

(define (autodoc-function-0)
  "the docstring")

(define (autodoc-function-1 x)
  "the docstring")

(define (repl-unit-tests)
  (load "demo/addons/s7/s7_scheme_repl.scm")

  ;; output is captured
  (assert-geiser-request
   ",geiser-eval #f (format #t \"~a~a\" 4 2) ()"
   '((result "\"42\"") (output . "42")))

  ;; define is evaluated at the top-level
  (assert-geiser-request
   ",geiser-eval #f (define *foo-bar* 13) ()"
   '((result "13") (output . "")))

  (assert-geiser-request
   ",geiser-eval #f (begin (format #t \"=> ~a\" *foo-bar*) *foo-bar*) ()"
   '((result "13") (output . "=> 13")))

  ;; can evaluate expression directly at the repl
  (assert-geiser-request-string
   "(format #t \"~a~%\" 42)"
   "42\n\n\"42\n\"")

  (assert-geiser-request-string
   ",geiser-eval #f ge:completions (\"completion-candidate\") ()"
   "((result \"(\\\"completion-candidate-1\\\" \\\"completion-candidate-2\\\")\") (output . \"\"))")

  ;; return the value when it's not a proc
  (assert-geiser-request
   ",geiser-eval #f ge:autodoc ('(autodoc-variable)) ()"
   '((result "((autodoc-variable (\"args\") (\"value\" . \"42\")))") (output . "")))

  (assert-geiser-request
   ",geiser-eval #f ge:autodoc ('(autodoc-function-1)) ()"
   '((result "((autodoc-function-1 (\"args\" ((\"required\" ...) (\"optional\") (\"key\")))))") (output . "")))

;;  < : scheme@(guile-user)> ((result "((hello (\"args\" ((\"required\" x) (\"optional\") (\"key\"))) (\"module\" guile-user)) (hello (\"args\" ((\"required\" x) (\"optional\") (\"key\"))) (\"module\" guile-user)))") (output . ""))

;;  > : ,geiser-eval #f ge:symbol-documentation ((quote hello)) ()
;;  < : scheme@(guile-user)> ((result "((\"signature\" hello (\"args\" ((\"required\" x) (\"optional\") (\"key\")))) (\"docstring\" . \"A procedure in module (guile-user).\\n\\nmy hello\"))") (output . ""))

;;  > : ,geiser-eval #f ge:symbol-location ((quote hello)) ()
;;  < : scheme@(guile-user)> ((result "((\"file\") (\"line\" . 48))") (output . ""))

  (assert-geiser-request-string
   "42"
   "42"))

(define (all-tests)
  (repl-unit-tests)
  (golden-scene-tests))

(catch #t
  (lambda ()
    (all-tests)
    (format #t "✅ Passed~%"))
  (lambda (type info)
    (let ((p (current-error-port)))
      (format p "❌ ")
      (apply format p info)
      (format p "~%"))
    (exit 1)))
