# Godot scripting via s7 Scheme

[Godot](https://godotengine.org/) integration for the wonderful [s7 Scheme](https://ccrma.stanford.edu/software/snd/snd/s7.html). `s7` interpreters can be added to scenes as `Scheme` nodes which can load and evaluate code.

The Scheme code has access to the Godot API via a simple interface (syntax is still in flux):
- the `Scheme` Godot node, which serves as an entry point into the scene model, is exposed as the `*node*` constant
- Godot nodes can be accessed through their relative path to `*node*` via the `$` macro, e.g. `($ Sprite2D)`
- properties can be read via applicable object syntax, e.g., `(*node* 'owner)` reads the [`owner` property](https://docs.godotengine.org/en/stable/classes/class_node.html#class-node-property-owner) of the enclosing node
- applicable syntax can also read nested properties, e.g., `(*node* 'owner 'name)` reads the name of the owner of the enclosing node
- applicable syntax can call methods, e.g., `(*node* 'owner '(get_child 0) 'name)`, even when the arguments are not constants, ``(*node* `(get_child ,child_index))``
- although explicit syntax for method calls is also provided for when it makes things clearer, `(! (*node* 'owner) 'get_child 0)`
- properties can be set via generalized `set!` syntax, e.g., `(set! (*node* 'owner '(get_child 0) 'name) "Deeply Nested set!")`
- Scheme code can connect to signals via `connect!`

A more complete example of the available syntax:

```scheme
(begin
  ;; Godot objects are exposed as applicable objects.
  (define button (*node* 'owner '(get_child 1)))

  (define (button-append! suffix)
    (let ((text (button 'text)))
      ;; Godot properties are set via generalized set! syntax
      ;; and there are two main ways of calling Godot methods:
      ;;  * (! <obj> <method symbol> &<args>)
      ;;  * (<obj> '(<method symbol> &<args>))
      ;; ! is preferred for effectful calls such as
      ;; 'insert below, and, in general is more amenable
      ;; to optimisations. Applicable object syntax
      ;; is convenient for const methods like '(length) below and
      ;; `(get_child 1) above.
      (set! (button 'text)
        (! text 'insert (text '(length)) suffix))))

  (define (function-handler)
    (button-append! "!"))

  (define (symbol-handler)
    (button-append! "'"))

  ;; Signals can be connected to symbols, lambdas and arbitrary procedures.
  ;; Symbols provide late binding, i.e., the ability to redefine the
  ;; procedure bound to a symbol / signal via the repl while the program is
  ;; running.
  (connect! button 'pressed 'symbol-handler)
  (connect! button 'pressed (lambda () (button-append! "λ")))
  (connect! button 'pressed function-handler))
```

## Status

Very experimental but a lot of fun to play with. Use it at your own risk.

## Building

Make sure to update all git submodules:

```shell
    git submodule update --init
```

Build and launch the demo project with `make run` or more explicitly via:

```shell
    scons && godot -e --path demo
```

Build the Android target with `make android` or more explicitly via:

```shell
  scons platform=android target=template_debug
```

Make sure `ANDROID_HOME` is set.

## Emacs live editing support (WIP)

Install [Geiser](https://www.nongnu.org/geiser/) then add the following to your Emacs configuration:

```elisp
(add-to-list 'load-path "~/path/to/godot-s7-scheme/emacs/")
(load "geiser-godot-s7-autoloads.el")
```

The Emacs extension automatically recognize Scheme files inside Godot project directories as `Godot s7 Scheme` files.

### Connecting to the editor

1. Start Godot with `--s7-tcp-port=<port-number>` (and/or `--s7-tcp-address=<bind-address>`).
2. Check the port number in the console.
3. In Emacs, `M-x connect-to-godot-s7`
3.1 You can also open a Scheme repl from the shell with `nc <bind-address> <port-number>`

### Connecting to a running scene

1. In Godot, select `Debug / Customize Run Instances... / Main Run Args`
   - Add `--s7-tcp-port=<port-number>` and/or `--s7-tcp-address=<bind-address>`.
2. Steps 2 and 3 as above.

## Roadmap

- [x] use Godot API from Scheme
- [x] live coding interface via Emacs
- [ ] expose tree-sitter API to Scheme
- [ ] Scheme editor with syntax highlighting
- [ ] Scheme notebooks
- [ ] expose Godot signals from Scheme
- [ ] subclass Godot classes from Scheme
- [ ] register Scheme as a proper [script language extension](https://docs.godotengine.org/en/stable/classes/class_scriptlanguageextension.html#class-scriptlanguageextension)
