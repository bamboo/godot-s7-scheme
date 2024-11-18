(provide 'prelude)

(define-macro* (inc! var (by 1))
  `(set! ,var (+ ,var ,by)))

(define-macro* ($ node-path (from *node*))
  (let ((node-path-str (symbol->string node-path)))
    `(! ,from 'get_node ,node-path-str)))

(define* (connect! object signal callable (flags 0))
  "Connects a procedure (or symbol that resolves to a procedure) to the signal of the given object."
  (! object 'connect (symbol->string signal) (Callable callable) flags))

(define (connected? obj signal symbol-or-procedure)
  (! obj 'is_connected (symbol->string signal) (Callable symbol-or-procedure)))

(define (disconnect! obj signal symbol-or-procedure)
  (! obj 'disconnect (symbol->string signal) (Callable symbol-or-procedure)))

(define (new class-symbol)
  (! (class-db) 'instantiate (symbol->string class-symbol)))

(define (load-resource resource-path)
  "Loads a Godot resource via the ResourceLoader."
  (let ((loader (new 'ResourceLoader)))
    (dynamic-wind
      (lambda () #f)
      (lambda () (! loader 'load resource-path))
      (lambda () (! loader 'free)))))

(define (load-scheme-resource resource-path)
  "Loads the given resource as a SchemeScript into the root environment."
  (let ((script (load-resource resource-path)))
    (print "Loading " (script 'resource_path) "...")
    (! *node* 'load script)))

(define (load-library lib-name)
  "Loads addons/s7/lib/<lib-name>.scm"
  (load-scheme-resource
   (string-append "res://addons/s7/lib/" lib-name ".scm")))

(define (require . symbols)
  "Loads a library from addons/s7/lib if it hasn't been loaded yet."
  (for-each
   (lambda (symbol)
     (when (not (provided? symbol))
       (load-library (symbol->string symbol))))
   symbols))

