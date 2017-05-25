(provide 'init.scm)

(set! (*s7* 'stacktrace-defaults)
      '(1000 ;; max-frames
        245 ;; code-cols
        455 ;; total-cols
        245  ;; "where to place comments"
        #t ;; whether the entire output should be displayed as a comment
        ))


(set! (*s7* 'history-size) 80)


;;
;; Important: Radium has not been initialized when this file is loaded.
;; Many of the ra: function does work though, but many will also crash the program
;; if accessed here.
;;
;; Code that uses ra: functions should be loaded in the function 'init-step-2'
;; in the end of this function.
;;


(define *is-initializing* #t)


(define (get-as-displayable-string-as-possible info)
  (define (fallback)
    (with-output-to-string (lambda ()
                             (display info))))
  (catch #t
         (lambda ()
           (cond ((defined? 'to-displayable-string)
                  (to-displayable-string info))
                 ((defined? 'pp)
                  (pp info))
                 (else
                  (fallback))))
         (lambda args
           (display "(get-as-displayable-string-as-possible info) failed. info:")
           (display info)
           (newline)
           (fallback))))


(define (safe-display-txt-as-displayable-as-possible txt)
  (display txt)
  (if (defined? 'safe-add-message-window-txt)
      (safe-add-message-window-txt txt)))

(define (history-ow!)
  (define history (copy (s7:get-history)))
  (call-with-output-string
   (lambda (p)
     ;; show history, if available
     (when (pair? history)
       (let ((history ())
             (lines ())
             (files ())
             (start history))
         (do ((x (cdr start) (cdr x))
              (i 0 (+ i 1)))
             ((or (eq? x start)
                  (null? (car x))
                  (= i (*s7* 'history-size))
                  (and (> i (- (*s7* 'history-size)
                               8))
                       (or (equal? (car x) '(history-ow!))
                           (equal? (car x) '(safe-history-ow!)))))
              (format p "~%history:~%    ~S" (if (pair? x) (car x) (car start)))
              (do ((x history (cdr x))
                   (line lines (cdr line))
                   (f files (cdr f)))
                  ((null? x))
                (format p (if (and (integer? (car line))
                                   (string? (car f))
                                   (not (string=? (car f) "*stdout*")))
                              (values "~%    ~S~40T;~A[~A]" (car x) (car f) (car line))
                              (values "~%    ~S" (car x)))))
              (format p "~%"))
           (set! history (cons (car x) history))
           (set! lines (cons (and (pair? (car x)) (pair-line-number (car x))) lines))
           (set! files (cons (and (pair? (car x)) (pair-filename (car x))) files))))))))

(define (safe-history-ow!)
  (catch #t
         (lambda ()
           (history-ow!)) ;; to strip down some unnecessary history produced by history-ow!
         (lambda args
           (get-as-displayable-string-as-possible (list "history-ow! failed: " args)))))

#!!
(safe-history-ow!)
!!#

(define (safe-display-history-ow!)
  (safe-display-txt-as-displayable-as-possible (safe-history-ow!)))

(define (handle-assertion-failure-during-startup info)
  (when *is-initializing*
    (display info)(newline)
    (catch #t
           (lambda ()
             (define infostring (get-as-displayable-string-as-possible info))
             (display "infostring: ")
             (display infostring)
             (newline)
             (ra:show-error infostring)
             )
           (lambda args
             (display "(Something failed 1. This is not good.)")(display args)(display (defined? 'ra:show-error))(newline)
             (safe-display-history-ow!)))
    (exit)))


;;(handle-assertion-failure-during-startup "testingexit")
;;(ra:show-error "hello")
;;;(exit)


;;(set! (*stacktrace* 'max-frames) 1000)
;(set! (*stacktrace* 'code-cols) 2000)
;(set! (*stacktrace* 'total-cols) 2450)

(define (get-full-path-of-scm-file filename)
  (let loop ((load-path *load-path*))
    (if (null? load-path)
        filename
        (let ((full-path (string-append (ra:get-program-path) "/" (car load-path) "/" filename)))
          (if (file-exists? full-path)
              full-path
              (loop (cdr load-path)))))))
  

(define *currently-loading-file* #f)
(define *currently-reloading-file* #f)

(set! (hook-functions *rootlet-redefinition-hook*)
      (list (lambda (hook)
              (let ((message (string-append "Warning: Redefining "
                                            (format #t "~A ~A~%" (hook 'name) (hook 'value))
                                            ;;(symbol->string (hook 'symbol))
                                            (if *currently-loading-file*
                                                (string-append " while loading " *currently-loading-file*)
                                                "."))))
                (cond (*is-initializing*
                       (display "Error during initializationg: ")
                       (display message)
                       (newline)
                       (catch #t
                              (lambda ()
                                (ra:add-message message))
                              (lambda args
                                #t))
                       (handle-assertion-failure-during-startup message))
                      ((and *currently-loading-file*
                            (not *currently-reloading-file*))
                       (ra:add-message message))
                      ((defined? 'c-display (rootlet))
                       (c-display message))
                      (else
                       (display message)
                       (newline)))))))


(define (my-equal? a b)
  (morally-equal? a b))
#||
  ;;(c-display "my-equal?" a b)
  (cond ((and (pair? a)
              (pair? b))
         (and (my-equal? (car a)
                         (car b))
              (my-equal? (cdr a)
                         (cdr b))))
        ((and (vector? a)
              (vector? b))
         (my-equal? (vector->list a)
                    (vector->list b)))
        ((and (procedure? a)
              (procedure? b))
         (structs-equal? a b))
        (else
         (morally-equal? a b))))
||#


;; assert and ***assert***
;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (assert something)
  (when (not something)
    (handle-assertion-failure-during-startup 'assert-failed) ;; we have a better call to handle-assertion-failure in ***assert***
    (throw 'assert-failed)))

;; It is assumed various places that eqv? can be used to compare functions.
(assert (eqv? assert ((lambda () assert))))


(define (***assert*** a b)
  (define (test -__Arg1 -__Arg2)
    (define (-__Func1)
      (let ((A -__Arg1))
        (if (my-equal? A -__Arg2)
            (begin
              (newline)
              (pretty-print "Correct: ")
              (pretty-print (to-displayable-string A))
              (pretty-print "")
              (newline)
              #t)
            (-__Func2))))
    (define (-__Func2)
      (let ((A -__Arg1))
        (let ((B -__Arg2))
          (begin
            (newline)
            (pretty-print "Wrong. Result: ")
            (pretty-print (to-displayable-string A))
            (pretty-print ". Correct: ")
            (pretty-print (to-displayable-string B))
            (pretty-print "")
            (handle-assertion-failure-during-startup (list "***assert*** failed. Result:" A ". Expected:" B))
            (newline)
            (throw 'assert-failed)
            #f))))
    (-__Func1))

  (test a b))




;; Redefine load so that we can show a warning window if redefining a symbol when loading a file for the first time
;;
(let ((org-load load))
  (set! load
        (let ((loaded-names (make-hash-table 39 string=?)))
          (lambda (filename . args)
            (set! filename (get-full-path-of-scm-file filename))
            (define env (if (null? args) #f (car args)))
            (define old-loading-filename *currently-loading-file*)
            (define old-reloading *currently-reloading-file*)
            (set! *currently-loading-file* filename)
            (let ((ret (catch #t
                              (lambda ()
                                (set! *currently-reloading-file* (loaded-names filename))
                                (hash-table-set! loaded-names filename #t)
                                (if env
                                    (org-load filename env)
                                    (org-load filename)))
                              (lambda args
                                (safe-display-history-ow!)))))
              (set! *currently-reloading-file* old-reloading)
              (set! *currently-loading-file* old-loading-filename)
              ret)))))



(require stuff.scm)

(define (safe-ow!)
  (catch #t
         ow!
         (lambda args
           (safe-display-history-ow!)
           (get-as-displayable-string-as-possible (list "ow! failed: " args)))))
  
(define (safe-display-ow!)
  (safe-display-txt-as-displayable-as-possible (safe-ow!)))

(require write.scm)

(define-constant go-wrong-finished-called-because-something-went-wrong 0)
(define-constant go-wrong-finished-called-because-it-was-excplicitly-called 1)

(define go-wrong-hooks '())

(assert (defined? 'safe-display-ow!))
(assert (defined? 'safe-ow!))
(set! (hook-functions *error-hook*) 
      (list (lambda (hook)
              (define backtrace-txt (safe-ow!))
              (safe-display-txt-as-displayable-as-possible backtrace-txt)
              (catch #t
                     (lambda ()
                       (let ((gwhs go-wrong-hooks))
                         (set! go-wrong-hooks '())
                         (for-each (lambda (go-wrong-hook)
                                     (go-wrong-hook))
                                   gwhs)))
                     (lambda args
                       (safe-display-history-ow!)
                       (get-as-displayable-string-as-possible (list "Custom error hook catch failed:\n"))))
              (handle-assertion-failure-during-startup (list "error-hook failed\n" backtrace-txt)))))

;;(handle-assertion-failure-during-startup "hello")

;; Test startup crash: (crash reporter should show up, and program exit). Startup crashes before this point (i.e. before the error hook is added, are not handled.)
;;(+ a 90)


(define-expansion (inc! var how-much)
  `(set! ,var (+ ,var ,how-much)))

(define-expansion (push! list el)
  `(set! ,list (cons ,el ,list)))

(define-expansion (push-back! list el)
  `(set! ,list (append ,list (list ,el))))


(define (delete-from das-list element)
  (assert (not (null? das-list)))
  (if (eqv? (car das-list) element)
      (cdr das-list)
      (cons (car das-list)
            (delete-from (cdr das-list) element))))

(define (delete-from2 das-list element)
  (assert (not (null? das-list)))
  (if (equal? (car das-list) element)
      (cdr das-list)
      (cons (car das-list)
            (delete-from (cdr das-list) element))))

(define (delete-list-from das-list elements)
  (if (null? elements)
      das-list
      (delete-list-from (delete-from das-list (car elements))
                        (cdr elements))))

;; This cleanup function will either be called when 'finished' is explicitly called,
;; or an exception happens. 'reason' will be the value of go-wrong-finished-called-because-something-went-wrong
;; or the value of go-wrong-finished-called-because-it-was-excplicitly-called.
;; Note that the argument for 'cleanup' can only be go-wrong-finished-called-because-something-went-wrong once.
;; See examples below.
(define (call-me-if-something-goes-wrong cleanup block)
  (define (remove-hook)
    (if (member? go-wrong-hook go-wrong-hooks)
        (set! go-wrong-hooks (delete-from go-wrong-hooks go-wrong-hook))))
  (define (go-wrong-hook)
    (remove-hook)
    (cleanup go-wrong-finished-called-because-something-went-wrong))
  (define (finished)
    (remove-hook)
    (cleanup go-wrong-finished-called-because-it-was-excplicitly-called))
  (push! go-wrong-hooks go-wrong-hook)
  (block finished))

#!!
(call-me-if-something-goes-wrong
 (lambda (reason)
   (assert (eqv? reason go-wrong-finished-called-because-it-was-excplicitly-called))
   (c-display "finished normally " reason))
 (lambda (finished)
   (finished)))

(call-me-if-something-goes-wrong
 (lambda (reason)
   (assert (eqv? reason go-wrong-finished-called-because-something-went-wrong))
   (c-display "finished because something went wrong " reason))
 (lambda (finished)
   (aasdeqrtpoiujqerotiuqpert)
   (finished)))
!!#


(define-constant *logtype-hold* (ra:get-logtype-hold))
(define-constant *logtype-linear* 0)


(load "common1.scm")

(define (my-require what)
  (if (provided? what)
      (c-display what "already provided")
      (load (get-full-path-of-scm-file (symbol->string what)))))

(my-require 'define-match.scm)

(my-require 'common2.scm)

(my-require 'nodes.scm)

;;(my-require 'mouse.scm)

(my-require 'instruments.scm)

(my-require 'fxrange.scm)

(my-require 'various.scm)

(my-require 'quantitize.scm)

(my-require 'timing.scm) ;; Note that the program assumes that g_scheme_has_inited1 is true after timing.scm has been loaded.


;; The files loaded in init-step-2 can use ra: functions.
(define (init-step-2)

  (set! *is-initializing* #t)

  ;; gui.scm can be loaded this late since expansion macros now can be defined after they are used.
  ;; Prev comment: Must be loaded early since it creates the <gui> expansion macro.
  (my-require 'gui.scm)
  
  (my-require 'mouse.scm)
  (my-require 'mixer-strips.scm)
  (my-require 'pluginmanager.scm)
  
  (set! *is-initializing* #f))


(set! *is-initializing* #f)




;;(gc #f)
