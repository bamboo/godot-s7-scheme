;;; geiser-godot-s7.el --- Godot s7 and Geiser talk to each other  -*- lexical-binding: t; -*-

;; Copyright (C) 2024 Rodrigo B. de Oliveira
;; Start date: Feb, 2024

;; Author: Rodrigo B. de Oliveira (rbo@acm.org)
;; Maintainer: Rodrigo B. de Oliveira (rbo@acm.org)
;; Keywords: languages, godot, s7, scheme, geiser
;; Homepage: https://github.com/bamboo/godot-s7-scheme
;; Package-Requires: ((emacs "25.1") (geiser "0.28.1"))
;; SPDX-License-Identifier: BSD-3-Clause
;; Version: 0.1.0

;; This file is NOT part of GNU Emacs.

;;; Commentary:

;; This package extends the `geiser' core package to support Godot s7.


;;; Code:

(require 'geiser-connection)
(require 'geiser-syntax)
(require 'geiser-custom)
(require 'geiser-repl)
(require 'geiser-debug)
(require 'geiser-impl)
(require 'geiser-base)
(require 'geiser-eval)
(require 'geiser-edit)
(require 'geiser-log)
(require 'geiser)

(require 'compile)
(require 'info-look)

(eval-when-compile
  (require 'cl-lib)
  (require 'subr-x))


;;; Customization

(defgroup geiser-godot-s7 nil
  "Customization for Geiser's Godot s7 flavour."
  :group 'geiser)

(geiser-custom--defcustom geiser-godot-s7-binary "godot"
  "Name to use to call the Godot executable when starting a REPL."
  :type '(choice string (repeat string)))

(geiser-custom--defcustom geiser-godot-s7-load-path nil
  "A list of paths to be added to Godot s7's load path when it's started.
The paths are added to both %`load-path' and %load-compiled path,
and only if they are not already present.  This variable is a
good candidate for an entry in your project's .dir-locals.el."
  :type '(repeat file))

(geiser-custom--defcustom geiser-godot-s7-init-file "~/.godot-s7"
  "Initialization file with user code for the Godot s7 REPL."
  :type 'string)

(geiser-custom--defcustom geiser-godot-s7-extra-keywords nil
  "Extra keywords highlighted in Godot s7 scheme buffers."
  :type '(repeat string))

(geiser-custom--defcustom geiser-godot-s7-manual-lookup-other-window nil
  "Non-nil means pop up the Info buffer in another window."
  :type 'boolean)

(geiser-custom--defcustom geiser-godot-s7-manual-lookup-nodes
    '("godotengine")
  "List of info nodes that, when present, are used for manual lookups."
  :type '(repeat string))


;;; REPL support

(defun geiser-godot-s7--binary ()
  "Return the name of the Godot s7 binary to execute."
  (if (listp geiser-godot-s7-binary)
      (car geiser-godot-s7-binary)
    geiser-godot-s7-binary))

(defvar geiser-godot-s7--conn-address "9998")

(defun geiser-godot-s7--parameters ()
  "Return a list with all parameters needed to start Godot s7."
  '("godot"))

(defconst geiser-godot-s7--prompt-regexp "^s7@([^)]*)> ")


;;; Evaluation support
(defsubst geiser-godot-s7--linearize-args (args)
  "Concatenate the list ARGS."
  (mapconcat 'identity args " "))

(defun geiser-godot-s7--geiser-procedure (proc &rest args)
  "Transform PROC in string for a scheme procedure using ARGS."
  (cl-case proc
    ((eval compile) (format ",geiser-eval %s %s%s"
                            (or (car args) "#f")
                            (geiser-godot-s7--linearize-args (cdr args))
                            (if (cddr args) "" " ()")))
    ((load-file compile-file) (format ",geiser-load-file %s" (car args)))
    ((no-values) ",geiser-no-values")
    (t (format "ge:%s (%s)" proc (geiser-godot-s7--linearize-args args)))))

(defun geiser-godot-s7--clean-up-output (str)
  str)

(defun geiser-godot-s7--get-module (&optional _module)
  "Find current buffer's module using MODULE as a hint."
  :f)

(defun geiser-godot-s7--module-cmd (module fmt &optional def)
  "Use FMT to format a change to MODULE, with default DEF."
  (when module
    (let* ((module (geiser-godot-s7--get-module module))
           (module (cond ((or (null module) (eq module :f)) def)
                         (t (format "%s" module)))))
      (and module (format fmt module)))))

(defun geiser-godot-s7--import-command (module)
  "Format a REPL command to use MODULE."
  (geiser-godot-s7--module-cmd module ",use %s"))

(defun geiser-godot-s7--enter-command (module)
  "Format a REPL command to enter MODULE."
  (geiser-godot-s7--module-cmd module ",m %s" "(godot-s7)"))

(defun geiser-godot-s7--exit-command ()
  "Format a REPL command to quit."
  ",q")

(defun geiser-godot-s7--symbol-begin (module)
  "Find beginning of symbol in the context of MODULE."
  (if module
      (max (save-excursion (beginning-of-line) (point))
           (save-excursion (skip-syntax-backward "^(>") (1- (point))))
    (save-excursion (skip-syntax-backward "^'-()>") (point))))


;;; Compilation shell regexps

(defconst geiser-godot-s7--path-rx "^In \\([^:\n ]+\\):\n")

(defconst geiser-godot-s7--rel-path-rx "^In +\\([^/\n: ]+\\):\n")

(defvar geiser-godot-s7--file-cache (make-hash-table :test 'equal)
  "Internal cache.")

(defun geiser-godot-s7--find-file (file)
  (or (gethash file geiser-godot-s7--file-cache)
      (with-current-buffer (or geiser-debug--sender-buffer (current-buffer))
        (when-let (r geiser-repl--repl)
          (with-current-buffer r
            (geiser-eval--send/result `(:eval (:ge find-file ,file))))))))

(defun geiser-godot-s7--resolve-file (file)
  "Find the given FILE, if it's indeed a file."
  (when (and (stringp file)
             (not (member file
                          '("socket" "stdin" "unknown file" "current input"))))
    (message "Resolving %s" file)
    (cond ((file-name-absolute-p file) file)
          (t (when-let (f (geiser-godot-s7--find-file file))
               (puthash file f geiser-godot-s7--file-cache))))))

(defun geiser-godot-s7--resolve-file-x ()
  "Check if last match contain a resolvable file."
  (let ((f (geiser-godot-s7--resolve-file (match-string-no-properties 1))))
    (and (stringp f) (list f))))


;;; Error display and debugger

(defun geiser-godot-s7--set-up-error-links ()
  (setq-local compilation-error-regexp-alist
              `((,geiser-godot-s7--path-rx geiser-godot-s7--resolve-file-x)
                ("^  +\\([0-9]+\\):\\([0-9]+\\)" nil 1 2)
                ("^\\(/.*\\):\\([0-9]+\\):\\([0-9]+\\)" 1 2 3)))
  (font-lock-add-keywords nil
                          `((,geiser-godot-s7--path-rx 1 compilation-error-face))))

(defun geiser-godot-s7--display-error (_module _key msg)
  "Display error with given message MSG."
  (when (stringp msg)
    (geiser-godot-s7--set-up-error-links)
    (save-excursion (insert msg)))
  (not (zerop (length msg))))


;;; Trying to ascertain whether a buffer is Godot s7 Scheme

(defun geiser-godot-s7--guess ()
  "Ascertain whether the file belongs to a Godot project."
  (locate-dominating-file (buffer-file-name) "project.godot"))


;;; Keywords and syntax

(defconst geiser-godot-s7--builtin-keywords
  '("call-with-input-file"
    "call-with-input-string"
    "call-with-output-file"
    "call-with-output-string"
    "with-output-to-string"
    "define*"
    "define-macro*"
    "define-bacro"
    "define-bacro*"
    "define-constant"
    "lambda*"
    "set!"
    "call!"
    "inc!"
    "connect!"
    "disconnect!"
    "$"
    "require"
    "provide"
    "import"
    "import-class"
    "define-signal"
    ))

(defun geiser-godot-s7--keywords ()
  "Return Godot s7-specific scheme keywords."
  (append
   (geiser-syntax--simple-keywords geiser-godot-s7-extra-keywords)
   (geiser-syntax--simple-keywords geiser-godot-s7--builtin-keywords)))

(geiser-syntax--scheme-indent
 (call-with-input-string 1)
 (call-with-output-string 0)
 (call-with-exit 0)
 (define* 1)
 (define-macro* 1)
 (define-bacro 1)
 (define-bacro* 1)
 (lambda* 1)
 (doto 1)
 (with-let 1)
 (when-let 1)
 (with-output-to-string 0))


;;; REPL startup

(defconst geiser-godot-s7-minimum-version "0.1")

(defun geiser-godot-s7--version (_binary)
  "Find Godot s7's version running the configured Godot s7 binary."
  geiser-godot-s7-minimum-version)

;;;###autoload
(defun connect-to-godot-s7 ()
  "Start a Godot s7 REPL connected to a remote process.

Start a Scheme Repl in the active Godot s7 scene."
  (interactive)
  (geiser-connect 'godot-s7))

(defun geiser-godot-s7--startup (_remote)
  "Startup function, for a remote connection if REMOTE is t."
  (geiser-godot-s7--set-up-error-links))


;;; Manual lookup

(defun geiser-godot-s7--info-spec ()
  "Return info specification for given NODES."
  (let* ((nrx "^[       ]+-+ [^:]+:[    ]*")
         (drx "\\b")
         (res (when (Info-find-file "r5rs" t)
                `(("(r5rs)Index" nil ,nrx ,drx)))))
    (dolist (node geiser-godot-s7-manual-lookup-nodes res)
      (when (Info-find-file node t)
        (mapc (lambda (idx)
                (add-to-list 'res
                             (list (format "(%s)%s" node idx) nil nrx drx)))
              '("R5RS Index" "Concept Index" "Procedure Index" "Variable Index", "Index"))))))

(info-lookup-add-help :topic 'symbol
                      :mode 'geiser-godot-s7-mode
                      :ignore-case nil
                      :regexp "[^()`',\"        \n]+"
                      :doc-spec (geiser-godot-s7--info-spec))

(defun geiser-godot-s7--info-lookup (id)
  (cond ((null id) (info "godotengine"))
        ((ignore-errors (info-lookup-symbol (format "%s" id) 'geiser-godot-s7-mode) t))
        ((and (listp id) (geiser-godot-s7--info-lookup (car (last id)))))
        (t (geiser-godot-s7--info-lookup (when (listp id) (butlast id))))))

(defun geiser-godot-s7--manual-look-up (id _mod)
  "Look for ID in the Godot s7 manuals."
  (let ((info-lookup-other-window-flag geiser-godot-s7-manual-lookup-other-window))
    (geiser-godot-s7--info-lookup id)
    (when geiser-godot-s7-manual-lookup-other-window
      (switch-to-buffer-other-window "*info*"))))


;;; debugging
(when nil
  (trace-function-foreground 'geiser-godot-s7--binary)
  (trace-function-foreground 'geiser-godot-s7--parameters)
  (trace-function-foreground 'geiser-godot-s7--version)
  (trace-function-foreground 'geiser-godot-s7--startup)
  (trace-function-foreground 'geiser-godot-s7--clean-up-output)
  (trace-function-foreground 'geiser-godot-s7--geiser-procedure)
  (trace-function-foreground 'geiser-godot-s7--guess)
  (trace-function-foreground 'geiser-godot-s7--symbol-begin)
  (trace-function-foreground 'geiser-godot-s7--enter-command)
  (trace-function-foreground 'geiser-godot-s7--exit-command)
  (trace-function-foreground 'geiser-godot-s7--get-module)
  (trace-function-foreground 'geiser-godot-s7--keywords)
  (trace-function-foreground 'geiser-godot-s7--display-error))

;;; Implementation definition:

(define-geiser-implementation godot-s7
                              (binary geiser-godot-s7--binary)
                              (arglist geiser-godot-s7--parameters)
                              (version-command geiser-godot-s7--version)
                              (minimum-version geiser-godot-s7-minimum-version)
                              (repl-startup geiser-godot-s7--startup)
                              (prompt-regexp geiser-godot-s7--prompt-regexp)
                              (clean-up-output geiser-godot-s7--clean-up-output)
                              (debugger-prompt-regexp nil)
                              (enter-debugger nil)
                              (marshall-procedure geiser-godot-s7--geiser-procedure)
                              (find-module geiser-godot-s7--get-module)
                              (enter-command geiser-godot-s7--enter-command)
                              (exit-command geiser-godot-s7--exit-command)
                              (import-command geiser-godot-s7--import-command)
                              (find-symbol-begin geiser-godot-s7--symbol-begin)
                              (display-error geiser-godot-s7--display-error)
                              (external-help nil)
                              (check-buffer geiser-godot-s7--guess)
                              (keywords geiser-godot-s7--keywords)
                              (case-sensitive :t)
                              (unsupported '(callers callees)))

;;;###autoload
(geiser-activate-implementation 'godot-s7)

;;;###autoload
(autoload 'run-godot-s7 "geiser-godot-s7" "Start a Geiser Godot s7 REPL." t)

;;;###autoload
(autoload 'switch-to-godot-s7 "geiser-godot-s7"
  "Start a Geiser Godot s7 REPL, or switch to a running one." t)

(provide 'geiser-godot-s7)
;;; geiser-godot-s7.el ends here
