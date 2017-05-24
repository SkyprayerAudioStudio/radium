(provide 'common2.scm)

#!!
(define-expansion (match-old args . matchers)
  (define matcher-func (gensym "matcher-func"))
  (eval
   `(let ()
      (define-match ,matcher-func
        ,@matchers)
      (apply ,matcher-func ,args))))
!!#

(define-expansion (match args . matchers)
  `(begin
     (eval (create-matcher-func 'matcher-func-temp-name ',matchers))
     (apply matcher-func-temp-name ,args)))


#!!
(pretty-print (macroexpand   (define-match is-define-lazy
                               (define-lazy _ _ ) :> #t
                               __________________ :> #f)))

(test (match (list 'a 'b)
             a b :> 5
             _ _ :> #f)
      5)
(match (list 'a 'b)
       a b :> 5
       _ :> 9)

(let-ref (rootlet) 'aiai2)
(defined? 'aiai (rootlet))

(procedure-source setaiai!)

(define (setaiai! val)
  (varlet (rootlet) 'aiai val))


!!#

(define (time)
  (*s7* 'cpu-time))


(define (keep func list)
  (if (null? list)
      '()
      (if (func (car list))
          (cons (car list)
                (keep func (cdr list)))
          (keep func (cdr list)))))

;;(keep (lambda (x) (= x 1)) (list 1 3 1 5))

(define (remove func list)
  (if (null? list)
      '()
      (if (func (car list))
          (remove func (cdr list))
          (cons (car list)
                (remove func (cdr list))))))

(define (sort sequence less?)
  (sort! (copy sequence) less?))

(define (flatten l)
  (cond ((null? l)
         '())
        ((pair? l)
         (append (flatten (car l))
                 (flatten (cdr l))))
        (else
         (list l))))

(define (get-bool something)
  (if something
      #t
      #f))

(define (scale x x1 x2 y1 y2)
  (+ y1 (/ (* (- x x1)
              (- y2 y1))
           (- x2 x1))))

(define (safe-scale x x1 x2 y1 y2)
  (let ((div (- x2 x1)))
    (if (= div 0)
        (begin
          (safe-add-message-window-txt (string-append "Error. Almost divided by zero in safe-scale: (= (- x2 x1) 0) " (number->string x2) " " (number->string x1)))
          0)
        (+ y1 (/ (* (- x x1)
                    (- y2 y1))
                 (- x2 x1))))))

(define (average . numbers)
  (/ (apply + numbers)
     (length numbers)))

(define (X/Y a b x y)
  (+ a (/ (* x (- b a))
          y)))

(define (onethird a b)
  (X/Y a b 1 3))
(define (twothirds a b)
  (X/Y a b 2 3))
         
(define (twofifths a b)
  (X/Y a b 2 5))
(define (threefifths a b)
  (X/Y a b 3 5))
         
;; fix max, which is buggy in s7 (the bug is most likely fixed now if you read this though)
#||
(define (max a . rest)
  (if (null? rest)
      a
      (let ((b (apply max rest)))
        (if (< a b)
            b
            a))))
||#

(assert (= 0 (max 0 -1/2)))

(define (between Min Try-it Max)
  (cond ((< Try-it Min)
         Min)
        ((> Try-it Max)
         Max)
        (else
         Try-it)))     

;; (round 2.5) -> 2
;; (roundup 2.5) -> 3
(define (roundup A)
  (floor (+ A 0.5)))

(define (two-decimals val)
  (/ (roundup (* val 100))
     100.0))

(define (one-decimal-string number)
  (format #f "~,1F" (* 1.0 number)))

(define (two-decimal-string number)
  (format #f "~,2F" (* 1.0 number)))

(define (one-decimal-percentage-string number)
  (format #f "~,1F" (* 100.0 number)))

(define (to-integer A)
  (inexact->exact (floor A)))

(define (to-list A)
  (if (vector? A)
      (vector->list A)
      A))

(define (min-notfalse . Args)
  (match (list Args)
         ()          :> #f
         (N)         :> N
         (#f . Rest) :> (apply min-notfalse Rest)
         (N  . Rest) :> (let ((that (apply min-notfalse Rest)))
                          (if that
                              (min N that)
                              N))))

#||
(test (min-notfalse)
      #f)
(test (min-notfalse #f)
      #f)
(test (min-notfalse #f 5)
      5)
(test (min-notfalse 5 #f)
      5)
(test (min-notfalse 8 #f 5)
      5)
||#

(define (max-notfalse . Args)
  (match (list Args)
         ()          :> #f
         (N)         :> N
         (#f . Rest) :> (apply max-notfalse Rest)
         (N  . Rest) :> (let ((that (apply max-notfalse Rest)))
                          (if that
                              (max N that)
                              N))))

#||
(test (max-notfalse)
      #f)
(test (max-notfalse #f)
      #f)
(test (max-notfalse #f 5)
      5)
(test (max-notfalse 5 #f)
      5)
(test (max-notfalse 8 #f 5)
      8)
||#



;; force and delay are missing from s7. Simple implementation below.
(define-expansion (delay . body)
  `(vector #f
           #f
           (lambda ()
             ,@body)))

(define (force something)
  (if (not (vector-ref something 0))
      (begin
        (vector-set! something 1 ((vector-ref something 2)))
        (vector-set! something 0 #t)))
  (vector-ref something 1))

#||
(define a (delay
            (c-display "hello")
            50))
(c-display a)
(force a)
||#


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;; define-struct ;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-match keyvalues-to-define-args
  ()                 :> '()
  (Key)              :> (cons (list (keyword->symbol Key) ''must-be-defined)
                              '())
  (Key1 Key2 . Rest) :> (cons (list (keyword->symbol Key1) ''must-be-defined)
                              (keyvalues-to-define-args (cons Key2 Rest)))
                        :where (keyword? Key2)                   
  (Key Value . Rest) :> (cons (list (keyword->symbol Key) Value)
                              (keyvalues-to-define-args Rest)))

#||
(test (keyvalues-to-define-args '(:a 90 :b 50 :c :d 80))
      '((a 90) (b 50) (c 'must-be-defined) (d 80)))
(test (keyvalues-to-define-args '(:a 90 :b 50 :c))
      '((a 90) (b 50) (c 'must-be-defined)))
||#


(define (copy-struct-helper original struct-name keys arguments)
  (if (keyword? original)
      (throw (<-> "Copy " struct-name " struct: First argument is not a struct, but a keyword")))

  ;; check that new data is valid
  (let loop ((arguments arguments))
    (if (not (null? arguments))
        (let ((key (car arguments))
              (value (cadr arguments)))
          (if (not (memq (keyword->symbol key) keys))
              (throw (<-displayable-> "key '" key (<-> "' not found in struct '" struct-name "'") ". keys: " (map symbol->keyword keys))))
          (loop (cddr arguments)))))

  (define new-table (copy original)) ;; No worries. 'new-table' will contain the "(cons eq? ,struct-mapper)" argument similar to 'original'.
  
  ;; add new data
  (let loop ((arguments arguments))
    (if (not (null? arguments))
        (let ((key (car arguments))
              (value (cadr arguments)))
          (hash-table-set! new-table key value)
          (loop (cddr arguments)))))

  new-table)


(define-expansion (define-struct name . args)
  (define define-args (keyvalues-to-define-args args))
  (define keys (map car define-args))
  (define keys-length (length keys))
  (define must-be-defined (keep (lambda (arg)
                                  (equal? ''must-be-defined (cadr arg)))
                                define-args))
  (define table (gensym "table"))
  (define key (gensym "key"))
  (define keysym (gensym "keysym"))
  (define ret (gensym "ret"))
  (define keysvar (gensym "keys"))
  (define keysvar2 (gensym "keys2"))
  (define original (gensym "original"))
  (define arguments (gensym "arguments"))
  (define loop (gensym "loop"))
  (define n (gensym "n"))

  (define struct-mapper (<_> name '-struct-mapper))
                             
  `(begin
     
     (define ,struct-mapper
       (let ((keytablemapper (make-hash-table ,keys-length eq?)))
         (for-each (lambda (key n)
                     (hash-table-set! keytablemapper (symbol->keyword key) n))
                   (quote ,keys)
                   (iota ,keys-length))
         (lambda (key)
           (or (keytablemapper key)
               (throw (<-displayable-> "key " (keyword->symbol key) ,(<-> " not found in struct '" name "'") ". keys: " (quote ,keys)))))))
     
     (define (,(<_> 'copy- name) ,original . ,arguments)
       (copy-struct-helper ,original
                           (quote ,name)
                           (quote ,keys)
                           ,arguments))
                     
     (define (,(<_> 'make- name '-nokeywords) ,@(map car (keyvalues-to-define-args args)))
       (let* ((,table (make-hash-table ,keys-length (cons eq? ,struct-mapper)))
              (,keysvar (quote ,keys)))
         ,@(map (lambda (key)
                  `(hash-table-set! ,table ,(symbol->keyword key) ,key))
                keys)
         ,table))

     (define* (,(<_> 'make- name) ,@(keyvalues-to-define-args args))
       ,@(map (lambda (must-be-defined)
                `(if (eq? ,(car must-be-defined) 'must-be-defined)
                     (throw ,(<-> "key '" (car must-be-defined) "' not defined when making struct '" name "'"))))
              must-be-defined)
       (let* ((,table (make-hash-table ,keys-length (cons eq? ,struct-mapper)))
              (,keysvar (quote ,keys)))
         ,@(map (lambda (key)
                  `(hash-table-set! ,table ,(symbol->keyword key) ,key))
                keys)
         ,table))))

#||
(define-struct test
  :b 59
  :c)

(make-test-nokeywords 3 4)
(make-test 3 4)

(define t (make-test :c 2))
(t :b)
(t :c)
(t :dir)

(define t2 (copy-test t :b 80))
(t2 :b)
(t2 :c)
(t2 :dir)


;; error, unknown key:
(copy-test t :unknown-key 2))


(define t2 (copy-test t :b 2))
(t2 :b)

(pretty-print (macroexpand (define-struct teststruct
                             :a 'asdf
                             :b
                             :c #f)))

(pretty-print (macroexpand
               (define-struct test
                 :b 59
                 :c)))

(hash-table* :a 9 :b 8)
           
(make-test :b 33)

(define t (make-test :c 2))
(t :b)
(t :c)
(t :dir)
(t :bc)
(t :b)

(hash-table-set! t :b 8)

(define t2 (t :copy :b 2))

(define tab (make-hash-table 32 eq?))
(hash-table-set! tab :hello 2)
(hash-table-set! tab :hello 3)
(tab :hello)


||#

;; doesn't "(morally-equal? hash-table1 hash-tabl2)" work? (not always, morally-equal? doesn't call my-equal?).
(define (structs-equal? a b)
  (morally-equal? a b))
#||
  (define alist-a a)
  (define alist-b b)
  (define keys-a (map car alist-a))
  
  (and (= (length keys-a)
          (length alist-b))
       (let loop ((keys-a keys-a))
         (if (null? keys-a)
             #t
             (and (my-equal? (a (car keys-a))
                             (b (car keys-a)))
                  (loop (cdr keys-a)))))))
||#




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;; delafina ;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



(define-match delafina-args-to-define*-args
  ()                 :> '()
  (Var . Rest)       :> (throw (<-> "All parameters for a delafina functions must be keywords. '" Var "' is not a keyword"))
                        :where (not (keyword? Var))
  (Key)              :> (list (keyword->symbol Key))
  (Key1 Key2 . Rest) :> (cons (keyword->symbol Key1)
                              (delafina-args-to-define*-args (cons Key2 Rest)))
                        :where (keyword? Key2)                   
  (Key Value . Rest) :> (cons (list (keyword->symbol Key) Value)
                              (delafina-args-to-define*-args Rest)))

(define-expansion (delafina def . body)
  `(define* (,(car def) ,@(delafina-args-to-define*-args (cdr def)))
     ,@body))

#||
(pretty-print (macroexpand (delafina (testfunc :b 30 :c 90)
                             (+ 2 3)
                             (+ 5 6))))


(pretty-print (macroexpand (delafina (testfunc :a :b :c 30 :d 90 :e f g)
                             (+ 2 3)
                             (+ 5 6))))

(test (macroexpand (delafina (testfunc a b :c 30 :d 90) a b))
      '(define* (testfunc a b (c 30) (d 90)) a b))

(define* (aiai (a 5) b (c 20))
  (list a b c))

(aiai)

(define* (aiai2 a b c)
  (list a b c))

(aiai2 2 3 4 5 6)
||#



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;; Try - Catch - Finally ;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;
#||
"try-finally" is NOT the same as dynamic-wind. It doesn't rethrow. I.e. the code following a call to "try-finally" should always run [1].

It would be more accurate to name the function something like "safe-try", "eat-errors", or "catch-all-errors-and-display-backtrace-automatically" etc.,
but I think the name "try-finally" is faster for the brain to understand when reading code.


Also note that the :finally thunk doesn't have an important purpose. It's just syntactic sugar. This:


(try-finally :try (lambda () 5)
             :finally newline)


...is the same as this:


(let ((ret (try-finally :try (lambda () 5))))
  (newline)
  ret)



[1] At least in the normal situations, not too sure about what happens if there is creative use of call/cc.
||#
;;
;;
;;
;; First a helper function:

(define-constant *try-finally-failed-return-value* (gensym "catch-all-errors-and-display-backtrace-automatically-failed-value"))

(define (FROM-C-catch-all-errors-and-display-backtrace-automatically func . args)
  (catch #t
         (lambda ()
           (apply func args))
         (lambda args
           (safe-display-ow!)
           *try-finally-failed-return-value*)))
  

(define (catch-all-errors-and-display-backtrace-automatically thunk)
  (catch #t
         thunk
         (lambda args
           (safe-display-ow!)
           *try-finally-failed-return-value*)))

(define (catch-all-errors-failed? ret)
  (eq? ret *try-finally-failed-return-value*))

;; Then the function we want to use everywhere:

(define-constant *try-finally-failure-thunk-failed* 'try-finally-failure-thunk-failed)
(define-constant *try-finally-false-unless-failure-is-overridden* 'false-unless-failure-is-overridden)

(delafina (try-finally :try
                       
                       :rethrow #f
                       
                       ;; If overridden, :failure will always return this value. If not overridden, the default :failure implementation will return #f.
                       :failure-return-value *try-finally-false-unless-failure-is-overridden*
                       
                       :failure (lambda ()
                                  (if (eq? failure-return-value *try-finally-false-unless-failure-is-overridden*)
                                      #f
                                      failure-return-value))
                       
                       :finally (lambda ()
                                  #f))
  (if rethrow
      (assert "rethrow not implemented"))
  
  (define (return ret)
    (finally)
    ret)
  
  (define try-ret (catch-all-errors-and-display-backtrace-automatically try))

  (if (catch-all-errors-failed? try-ret)
      (begin                    
        (define failed-ret (catch-all-errors-and-display-backtrace-automatically failure))
        (cond ((catch-all-errors-failed? failed-ret)
               (return *try-finally-failure-thunk-failed*))
              ((eq? failure-return-value *try-finally-false-unless-failure-is-overridden*)
               (return failed-ret))
              (else
               (return failure-return-value))))
      (return try-ret)))



(c-display "\n\n\n\n=======================================================================================================")
(c-display "    START testing try-catch-failure. Lots of backtrace will be printed, but nothing is wrong, hopefully.")
(c-display "=======================================================================================================\n\n\n\n")


;; Test all fine.
(***assert*** (try-finally :try (lambda ()
                                  (c-display "returning 5")
                                  (+ 2 3))
                           :rethrow #f)
              5)

;; Test all fine with finalizer
(let ((is-finalized #f))
  (***assert*** (try-finally :try (lambda ()
                                    (c-display "returning 5")
                                    (+ 2 3))
                             :finally (lambda ()
                                        (c-display "finally")
                                        (set! is-finalized #t)))
                5)
  (***assert*** is-finalized #t))

;; Test all fine and catch not called.
(let ((is-finalized #f)
      (is-catched #f))
  (***assert*** (try-finally :try (lambda ()
                                    (c-display "returning 5")
                                    (***assert*** is-finalized #f)
                                    (+ 2 3))
                             :failure (lambda ()
                                        (c-display "catching")
                                        (***assert*** is-finalized #f)
                                        (set! is-catched #t))
                             :finally (lambda ()
                                        (c-display "finally")
                                        (***assert*** is-catched #f)
                                        (set! is-finalized #t)))
                5)
  (***assert*** is-finalized #t)
  (***assert*** is-catched #f))


;; Test failure in 'try'. Returns #f by default.
(let ((is-finalized #f))
  (define result (try-finally :try (lambda ()
                                     (c-display "returning 5")
                                     (***assert*** is-finalized #f)
                                     (+ a 3))
                              :failure-return-value 281
                              :finally (lambda ()
                                         (c-display "finally")
                                         (set! is-finalized #t))))
  (***assert*** result 281)
  (***assert*** is-finalized #t))


;; Test failure in 'try'. Custom failure function.
(let ((is-finalized #f))
  (define result (try-finally :try (lambda ()
                                     (c-display "returning 5")
                                     (***assert*** is-finalized #f)
                                     (+ a 3))
                              :finally (lambda ()
                                         (c-display "finally")
                                         (set! is-finalized #t))))
  (***assert*** result #f)
  (***assert*** is-finalized #t))


;; Test failure in 'try'. Custom failure func.
(let ((is-finalized #f)
      (is-catched #f))

  (define result (try-finally :try (lambda ()
                                     (c-display "returning 5")
                                     (***assert*** is-finalized #f)
                                     (+ a 3))
                              :failure (lambda ()
                                         (c-display "catching")
                                         (***assert*** is-finalized #f)
                                         (set! is-catched #t)
                                         'failed)
                              :finally (lambda ()
                                         (c-display "finally")
                                         (***assert*** is-catched #t)
                                         (set! is-finalized #t))))
  (***assert*** result 'failed)
  (***assert*** is-finalized #t)
  (***assert*** is-catched #t))


;; Test failure in 'try'. Custom failure function and custom failure return value.
(let ((is-finalized #f)
      (is-catched #f))
  (define result (try-finally :try (lambda ()
                                     (c-display "returning 5")
                                     (***assert*** is-finalized #f)
                                     (+ a 3))
                              :failure-return-value 281
                              :failure (lambda ()
                                         (c-display "catching")
                                         (***assert*** is-finalized #f)
                                         (set! is-catched #t)
                                         'failed)
                              :finally (lambda ()
                                         (c-display "finally")
                                         (***assert*** is-catched #t)
                                         (set! is-finalized #t))))
  (***assert*** result 281)
  (***assert*** is-catched #t)
  (***assert*** is-finalized #t))


;; Test failure in 'failure'. (a lot of backtrace is supposed to be printed now, but the important thing is that the last three asserts are correct.)
(let ((is-finalized #f)
      (is-catched #f))
  (define result (try-finally :try (lambda ()
                                     (c-display "returning 5")
                                     (***assert*** is-finalized #f)
                                     (+ a 3))
                              :failure (lambda ()
                                         (c-display "catching")
                                         (***assert*** is-finalized #f)
                                         (set! is-catched #t)
                                         (+ b 4)
                                         'failed)
                              :finally (lambda ()
                                         (c-display "finally")
                                         (***assert*** is-catched #t)
                                         (set! is-finalized #t))))
  (***assert*** result *try-finally-failure-thunk-failed*)
  (***assert*** is-finalized #t)
  (***assert*** is-catched #t))

;; Test failure in 'finally'. (we test that a failure in 'finally' is not caught)
(let ((is-finalized #f)
      (is-catched #f)
      (finally-failed #f))
  (define result (catch #t
                        (lambda ()
                          (try-finally :try (lambda ()
                                              (c-display "returning 5")
                                              (***assert*** is-finalized #f)
                                              (+ a 3))
                                       :failure (lambda ()
                                                  (c-display "catching")
                                                  (***assert*** is-finalized #f)
                                                  (set! is-catched #t)
                                                  (+ b 4)
                                                  'failed)
                                       :finally (lambda ()
                                                  (c-display "finally")
                                                  (***assert*** is-catched #t)
                                                  (set! is-finalized #t)
                                                  (+ c 5))))
                        (lambda args
                          (set! finally-failed #t)
                          'finally-failed2)))
  
  (***assert*** result 'finally-failed2)
  (***assert*** finally-failed #t)
  (***assert*** is-finalized #t)
  (***assert*** is-catched #t))


(c-display "\n\n\n\n=======================================================================================================")
(c-display "    ENDED testing try-catch-failure. Lots of backtrace was printed, but nothing is wrong, hopefully.")
(c-display "=======================================================================================================\n\n\n\n")



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;; Box handling ;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-struct box :x :y :x1 :y1 :x2 :y2 :width :height)

(define (make-box2 $x1 $y1 $x2 $y2)
  (make-box :x (average $x1 $x2)
            :y (average $y1 $y2)
            :x1 $x1
            :y1 $y1
            :x2 $x2
            :y2 $y2
            :width (- $x2 $x1)
            :height (- $y2 $y1)))
  
(define-macro (ra:get-box prefix . rest)
  `(make-box2 ( ,(<_> 'ra:get- prefix '-x1) ,@rest)
              ( ,(<_> 'ra:get- prefix '-y1) ,@rest)
              ( ,(<_> 'ra:get- prefix '-x2) ,@rest)
              ( ,(<_> 'ra:get- prefix '-y2) ,@rest)))

(define-expansion (ra:get-box2 prefix . rest)
  `(make-box2 ( ,(<_> 'ra:get- prefix '-x1) ,@rest)
              ( ,(<_> 'ra:get- prefix '-y1) ,@rest)
              ( ,(<_> 'ra:get- prefix '-x2) ,@rest)
              ( ,(<_> 'ra:get- prefix '-y2) ,@rest)))


(define (box-to-string box)
  (if (not box)
      "<box is #f>"
      (<-> "(box"
           " :x1 "     (box :x1)
           " :y1 "     (box :y1)
           " :x2 "     (box :x2)
           " :y2 "     (box :y2)
           " :width "  (box :width)
           " :height " (box :height)
           ")")))

#||
(pretty-print (macroexpand (define-struct box :x1 :y1 :x2 :y2)))

(list
 ((ra:get-box reltempo-slider) :x1)
 ((ra:get-box reltempo-slider) :y1)
 ((ra:get-box reltempo-slider) :x2)
 ((ra:get-box reltempo-slider) :y2)
 ((ra:get-box reltempo-slider) :width)
 ((ra:get-box reltempo-slider) :height))
||#

(define (inside-box box x y)
  ;;(c-display "Whtas the box:" box)
  (and (>= x (box :x1))
       (<  x (box :x2))
       (>= y (box :y1))
       (<  y (box :y2))))

#||
||#

(define (inside-box-forgiving Box X Y) ;; Inside a box, inluding half the width of a node.
  (define width/2 (1+ (ra:get-half-of-node-width)))
  (and (>= X (- (Box :x1) width/2))
       (<  X (+ (Box :x2) width/2))
       (>= Y (- (Box :y1) width/2))
       (<  Y (+ (Box :y2) width/2))))


;; Replaces all occurences of A with B in List
(define-match deep-list-replace
  A B A          :> B
  _ _ (        ) :> '()
  A B (R . Rest) :> (cons (deep-list-replace A B R)
                          (deep-list-replace A B Rest))
  A B C          :> C)
                           
#||
(test (deep-list-replace 1 2 1)
      2)
(test (deep-list-replace '() 2 '())
      2)
(test (deep-list-replace 1 2 3)
      3)
(test (deep-list-replace 1 2 '(1 1))
      '(2 2))
(test (deep-list-replace 1 2 '(1 1 . 1))
      '(2 2 . 2))
(test (deep-list-replace 1 2 '(1 (1 . 1) 2 (2 . 1) (3 1)))
      '(2 (2 . 2) 2 (2 . 2) (3 2)))
||#

(define-match deep-list-replace-several
  ()            List :> List
  ((A B) . ABs) List :> (deep-list-replace-several ABs
                                                   (deep-list-replace A B List)))

#||
(test (deep-list-replace-several '((1 2)(3 4)) '(1 3))
      '(2 4))
(test (deep-list-replace-several '((a (force a))) '(+ a a a))
      '(+ (force a) (force a) (force a)))
||#


#||
for .emacs:

(font-lock-add-keywords
 'scheme-mode
 '(("(\\(define-lazy\\)\\>\\s-*(?\\(\\sw+\\)?"
    (1 font-lock-keyword-face)
    (2 font-lock-type-face)
    (3 (cond ((match-beginning 1) font-lock-function-name-face)
	     ((match-beginning 2) font-lock-variable-name-face)
	     ((match-beginning 3) font-lock-function-name-face)
	     (t font-lock-type-face))
       nil t))))

(font-lock-add-keywords
 'scheme-mode
 '(("(\\(lazy\\)\\>\\s-*(?\\(\\sw+\\)?"
    (1 font-lock-keyword-face)
    (2 (cond ((match-beginning 1) font-lock-function-name-face)
	     ((match-beginning 2) font-lock-variable-name-face)
	     (t font-lock-type-face))
       nil t))))
||#

(define-match is-define-lazy  
  (define-lazy _ _ ) :> #t
  __________________ :> #f)

(define-match get-lazy-replacement
  (define-lazy Name _____) :> `(,Name (force ,Name))
  ________________________ :> (throw 'something-went-wrong-in-get-lazy-replacement-in-lazy))
  
(define-match transform-lazy-code
  Replacements (define-lazy Name Value) :> `(define ,Name (delay ,(deep-list-replace-several Replacements Value)))
  ____________ _________________________ :> #f)
  

(define-expansion (lazy . body)
  ;;(c-display "EXPSNDFING lazy macro")
  
  (define lazy-vals (keep is-define-lazy body))
  (define lazy-replacements (map get-lazy-replacement lazy-vals))
  (define lazy-vals-code (map (lambda (lazy-val)
                                (transform-lazy-code lazy-replacements lazy-val))
                              lazy-vals))
  
  (define rest-body (remove is-define-lazy body))
  (define rest-body-code (deep-list-replace-several lazy-replacements rest-body))
  
  `(begin
     ,@lazy-vals-code
     ,@rest-body-code))

#||
(test (lazy
       (define-lazy a 50)
       (define-lazy b 60)
       (+ a b))
      110)

(define-expansion (lazy2 . body)
  (define-match hepp
    _ :> "hepp")

  `(begin
     (c-display ,(hepp #t))
     ,@body))

(define (hepp)
  (lazy2
    ;;(define-lazy a 50)
    60))

(hepp)

(lazy2 5)

(macroexpand (lazy 60))

(test (let ((val 0))
        (lazy
         (define-lazy a (begin
                          (set! val (+ val 1))
                          val))
         (+ a a a)))
      3)

(test (lazy
        (define-lazy a 5)
        (define-lazy b a)
        b)
      5)

(pretty-print (macroexpand (lazy
                             (define-lazy a 5)
                             (define-lazy b a)
                             b)))

(macroexpand (lazy
              (define-lazy a (begin
                               (set! val (+ val 1))
                               val))
              (+ a a a)))

(macroexpand (lazy
              (define-lazy a 50)
              (define-lazy b 60)
              (+ a b)))
||#


(define-expansion (<ra> command . args)
  `( ,(<_> 'ra: (keyword->symbol command)) ,@args))

                              

(define (group-by get-key key-compare elements)
  (define keys '())
  (define hash (make-hash-table 39 key-compare))
  (for-each (lambda (element)
              (let* ((key (get-key element))
                     (old-value (hash-table-ref hash key)))
                (if (not old-value)
                    (push-back! keys key))
                (hash-table-set! hash
                                 key
                                 (cons element
                                       (or old-value '())))))
            elements)
  (map (lambda (key)
         (reverse! (hash-table-ref hash key)))
       keys))

(***assert*** (group-by (lambda (x)
                          x)
                        =
                        '(1 5 2 3 5 1))
              '((1 1)
                (5 5)
                (2)
                (3)))

(define (true-for-all? pred elements)
  (cond ((null? elements)
         #t)
        ((pred (car elements))
         (true-for-all? pred (cdr elements)))
        (else
         #f)))
               

(***assert*** (true-for-all? even? '())
              #t)

(***assert*** (true-for-all? even? '(2 4 6))
              #t)

(***assert*** (true-for-all? even? '(2 4 3))
              #f)


(define (true-for-at-least-one? pred elements)
  (if (null? elements)
      #f
      (or (pred (car elements))
          (true-for-at-least-one? pred (cdr elements)))))

(***assert*** (true-for-at-least-one? even? '())
              #f)

(***assert*** (true-for-at-least-one? even? '(2 4 6))
              #t)

(***assert*** (true-for-at-least-one? even? '(2 4 3))
              #t)

(***assert*** (true-for-at-least-one? even? '(1 9 3))
              #f)

;; Precompute all iota results up to, and including, 1024.
(let* ((org-iota iota)
       (iota-results (list->vector (map (lambda (n)
                                          (org-iota n))
                                        (org-iota 1026)))))
  ;;(c-display "res" iota-results)
  (set! iota
        (lambda (n)
          (if (< n 1025)
              (iota-results n)
              (org-iota n)))))


(define (vector-copy vector)
  (copy vector))

(define (cl-car a)
  (if (pair? a)
      (car a)
      #f))

(define (cl-cdr a)
  (if (pair? a)
      (cdr a)
      #f))

(define (cl-cadr a)
  (cl-car (cl-cdr a)))

(define (cl-caddr a)
  (cl-car (cl-cdr (cl-cdr a))))

(define (butlast elements)
  (let ((rest (cdr elements)))
    (if (null? rest)
        '()
        (cons (car elements)
              (butlast rest)))))

(***assert*** (butlast '(2)) '())
(***assert*** (butlast '(2 3)) '(2))


(define (second-last elements)
  (cadr (reverse elements)))

;; like list-set! except that it doesn't modify the list
(define (list-replace-element das-list pos new-value)
  (if (= 0 pos)
      (cons new-value
            (cdr das-list))
      (cons (car das-list)
            (list-replace-element (cdr das-list)
                                  (1- pos)
                                  new-value))))

(define (list-remove das-list pos)
  (if (= 0 pos)
      (cdr das-list)
      (cons (car das-list)
            (list-remove (cdr das-list)
                         (1- pos)))))

(***assert*** (list-remove '(0 1 2) 0) '(1 2))
(***assert*** (list-remove '(0 1 2) 1) '(0 2))
(***assert*** (list-remove '(0 1 2) 2) '(0 1))


(define (remove-duplicates-in-sorted-list comparer das-list)
  (if (null? das-list)
      '()
      (let ((a (car das-list)))
        (if (null? (cdr das-list))
            das-list
            (let ((b (cadr das-list)))
              (if (comparer a b)
                  (remove-duplicates-in-sorted-list comparer (cdr das-list))
                  (cons a
                        (remove-duplicates-in-sorted-list comparer (cdr das-list)))))))))
                   

(define (integer-range start-inclusive end-inclusive)
  (map (lambda (i)
         (+ i start-inclusive))
       (iota (1+ (- end-inclusive start-inclusive)))))

(***assert*** (integer-range 0 5)
              '(0 1 2 3 4 5))
(***assert*** (integer-range 5 5)
              '(5))



;;;;;;;;;; popup menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; a 50 b 90 c 100 -> '((a 50)(b 90)(c 100))
(define-match make-assoc-from-flat-list
  ()           :> '()
  (A B . Rest) :> (cons (list A B)
                        (make-assoc-from-flat-list Rest)))

#||
(make-assoc-from-flat-list (list "a" 50 "b" 90 "c" 100))
||#

(define (string-starts-with? string startswith)
  (define (loop string startswith)
    (cond ((null? startswith)
           #t)
          ((null? string)
           #f)
          ((char=? (car string) (car startswith))
           (loop (cdr string) (cdr startswith)))
          (else
           #f)))
  (loop (string->list string)
        (string->list startswith)))

(***assert*** (string-starts-with? "" "") #t)
(***assert*** (string-starts-with? "asdf" "as") #t)
(***assert*** (string-starts-with? "asdf" "") #t)
(***assert*** (string-starts-with? "" "a") #f)
(***assert*** (string-starts-with? "a" "a") #t)
(***assert*** (string-starts-with? "a" "b") #f)
(***assert*** (string-starts-with? "ab" "a") #t)

(define (string-ends-with? string endswith)
  (define (loop string startswith)
    (cond ((null? startswith)
           #t)
          ((null? string)
           #f)
          ((char=? (car string) (car startswith))
           (loop (cdr string) (cdr startswith)))
          (else
           #f)))
  (loop (reverse (string->list string))
        (reverse (string->list endswith))))

(***assert*** (string-ends-with? "" "") #t)
(***assert*** (string-ends-with? "asdf" "df") #t)
(***assert*** (string-ends-with? "asdf" "") #t)
(***assert*** (string-ends-with? "" "a") #f)
(***assert*** (string-ends-with? "a" "a") #t)
(***assert*** (string-ends-with? "a" "b") #f)
(***assert*** (string-ends-with? "ab" "b") #t)

;; Returns true if bb is placed inside aa.
(define (string-contains? aa bb)
  (if (or (string=? bb "")
          (string-position bb aa))
      #t
      #f))
#||
  (if (string=? bb "")
      #t
      (begin
        (define b (bb 0))
        (let loop ((aa (string->list aa)))
          (cond ((null? aa)
                 #f)
                ((and (char=? b (car aa))
                      (string-starts-with? (list->string aa) bb))
                 #t)
                (else
                 (loop (cdr aa))))))))
  ||#
  
(***assert*** (string-contains? "" "") #t)
(***assert*** (string-contains? "asdf" "df") #t)
(***assert*** (string-contains? "asdf" "") #t)
(***assert*** (string-contains? "" "a") #f)
(***assert*** (string-contains? "a" "a") #t)
(***assert*** (string-contains? "a" "b") #f)
(***assert*** (string-contains? "ab" "b") #t)
(***assert*** (string-contains? "abcd" "bc") #t)
(***assert*** (string-contains? "abccb" "bcd") #f)
(***assert*** (string-contains? "abbcd" "bcd") #t)

(define (string-case-insensitive-contains? aa bb)
  (string-contains? (string-upcase aa) (string-upcase bb)))

(define (parse-popup-menu-options args)
  (if (null? args)
      '()
      (cond ((list? (car args))
             (parse-popup-menu-options (append (car args)
                                               (cdr args))))
            ((not (car args))
             (parse-popup-menu-options (cdr args)))
            ((string-starts-with? (car args) "---")
             (cons (car args)
                   (cons (lambda _ #t)
                         (parse-popup-menu-options (cdr args)))))
            (else
             (assert (not (null? (cdr args))))
             (let ((text (car args))
                   (arg2 (cadr args)))
               (cond ((eq? :check arg2)
                      (let ((check-on (caddr args)))
                        (parse-popup-menu-options (cons (<-> (if check-on "[check on]" "[check off]") text)
                                                        (cdddr args)))))
                     ((eq? :enabled arg2)
                      (let ((enabled (caddr args)))
                        (if enabled
                            (parse-popup-menu-options (cons text
                                                            (cdddr args)))
                            (parse-popup-menu-options (cons (<-> "[disabled]" text)
                                                            (cdddr args))))))
                     ((procedure? arg2)
                      (cons text
                            (cons arg2
                                  (parse-popup-menu-options (cddr args)))))
                     ((list? arg2)
                      (append (list (<-> "[submenu start]" text)
                                    (lambda () #t))
                              (parse-popup-menu-options arg2)
                              (list "[submenu end]"
                                    (lambda () #t))
                              (parse-popup-menu-options (cddr args))))))))))

#||
(parse-popup-menu-options (list (list "bbb" (lambda ()
                                              6))
                                "------"))

(parse-popup-menu-options (list "hello1" :enabled #t (lambda ()
                                                       (c-display "hepp1"))
                                "hello2" :enabled #f (lambda ()
                                                       (c-display "hepp2"))                                
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

(parse-popup-menu-options (list "hello1" :check #t (lambda ()
                                                     (c-display "hepp1"))                                
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

(parse-popup-menu-options (list "hello1" :check #f (lambda ()
                                                     (c-display "hepp1"))                                
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))

(parse-popup-menu-options (list "hello1" (lambda ()
                                           (c-display "hepp1"))                                
                                "submenu" (list
                                           "hello2" (lambda ()
                                                      (c-display "hepp2"))
                                           "hello3" (lambda ()
                                                      (c-display "hepp3")))
                                "hello4" (lambda ()
                                           (c-display "hepp4"))))
||#

(define (get-popup-menu-args args)
  ;;(c-display "aaa")
  (define options (parse-popup-menu-options args))
  ;;(c-display "bbb")
  ;;(c-display "optinos:" options)
  (define relations (make-assoc-from-flat-list options))
  ;;(c-display "ccc")
  (define strings (map car relations))
  ;;(define strings (list->vector (map car relations)))
  ;;
  ;;(define popup-arg (let loop ((strings (vector->list strings)))
  ;;                    ;;(c-display "strings" strings)
  ;;                    (if (null? strings)
  ;;                        ""
  ;;                        (<-> (car strings) " % " (loop (cdr strings))))))
  
  ;;(c-display "   relations: " relations)
  ;;(for-each c-display relations (iota (length relations)))
  ;;(c-display "strings: " strings)
  ;;(c-display "popup-arg: " popup-arg)
  
  (define (get-func n)
    ;;(c-display "N: " n)
    ;;(define result-string (vector-ref strings n))
    ;;(cadr (assoc result-string relations))
    (cadr (relations n))
    )

  (list strings
        (lambda (n . checkboxval)
          (define result-string (strings n))
          (if (null? checkboxval)
              ((get-func n))
              ((get-func n) (car checkboxval))))))

(define (popup-menu-from-args popup-menu-args)
  (apply ra:popup-menu popup-menu-args))
        
;; Async only. Use ra:simple-popup-menu for sync.
(define (popup-menu . args)
  (popup-menu-from-args (get-popup-menu-args args)))


#||
(popup-menu (list "Select preset from a different directory"
                  (lambda x
                    x))
            "------------"
            (map (lambda (base64-name)
                   (list (<ra> :from-base64 base64-name)
                         (lambda ()
                           (callback (<-> "2" base64-name)))))
                 (list "asdf" "bzcv")))

(popup-menu (list (list "aaa" (lambda ()
                                5))
                  "----"
                  (list "bbb" (lambda ()
                                6))
                  "----------"))

(popup-menu "aaa" (lambda ()
                    (c-display "main menu"))
            "bbb" (list "aaa"
                        (lambda ()
                          (c-display "submenu"))))

(popup-menu "hello" :check #t (lambda (ison)
                                (c-display "gakk1" ison))
            "hello2" :enabled #t (lambda ()
                                   (c-display "gakk2"))
            "hello3" :enabled #f (lambda ()
                                   (c-display "gakk3"))
            )
||#
            
#||
(popup-menu "[check on] gakk1 on" (lambda (ison)
                                   (c-display "gakk1 " ison))
            "[check off] gakk2 off" (lambda (ison)
                                     (c-display "gakk2 " ison))
            "hepp1" (lambda ()
                     (c-display "hepp1"))
            "hepp2" (lambda ()
                     (c-display "hepp2"))
            )

(popup-menu "hello" (lambda ()
                      (c-display "hepp"))
            "[submenu start]Gakk gakk-" (lambda () #t)
            "[submenu start]Gakk gakk-" (lambda () #t)
            "hello2" (lambda ()
                       (c-display "hepp2"))
            "[submenu end]" (lambda () #t)
            "[submenu end]" (lambda () #t)
            "hepp" (lambda ()
                     (c-display "hepp3")))
(popup-menu "hello" (lambda ()
                      (c-display "hepp"))
            "Gakk gakk" (list
                         "Gakk gakk2" (list
                                       "hello2" (lambda ()
                                                  (c-display "hepp2"))
                                       "hello3" (lambda ()
                                                  (c-display "hepp3"))))
            "hepp" (lambda ()
                     (c-display "hepp3")))
||#

(define-constant *num-radium-ticks* (<ra> :get-highest-legal-place-denominator))
(define-constant *smallest-radium-tick* (/ 1 *num-radium-ticks*))
(define (-line linenum)
  (- linenum *smallest-radium-tick*))

#||
(define (+line linenum)
  (+ linenum *smallest-radium-tick*))
||#


(define (undo-block block)
  (<ra> :open-undo)
  (let ((ret (catch #t
                    block
                    (lambda args ;; Catch exceptions to ensure (<ra> :cose-undo) will be called
                      (display "args")(display args)(newline)
                      (apply format #t (cadr args))
                      (display (ow!))))))
    (<ra> :close-undo)
    ret))

(define (ignore-undo-block block)
  (<ra> :start-ignoring-undo)
  (let ((ret (catch #t
                    block
                    (lambda args ;; Catch exceptions to ensure (<ra> :cose-undo) will be called
                      (display "args")(display args)(newline)
                      (apply format #t (cadr args))
                      (display (ow!))))))
    (<ra> :stop-ignoring-undo)
    ret))

(define (draw-plot xs func)
  (define ys (map func xs))
  (define args "")
  (for-each (lambda (x y)
              (set! args (<-> args " " (* 1.0 x) " " (* 1.0 y) )))
            xs ys)
  (system (<-> "plot" args "&"))
  ;;(read-char)
  (c-display "args" args)
  ys)


#||
(define (fibgakk arg)
  (if (= arg 0)
      (<ra> :testsomething arg)
      (fibgakk (1- arg))))

(define (test-crash)
  (fibgakk 50))
||#



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Coroutines (missing yield and so forth, but we don't need it it, at least not yet)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-struct coroutine
  :func #f
  :is-scheduled #f
  :is-running #f
  :please-stop-me #f)

(define (coroutine-alive? coroutine)
  ;;(c-display "coroutine:" coroutine (coroutine :is-scheduled))
  (or (coroutine :is-scheduled)
      (coroutine :is-running)))

(define (stop-coroutine! coroutine)
  (cond ((coroutine :is-scheduled)
         (let ((func (coroutine :func)))
           (<ra> :remove-schedule func)
           (set! (coroutine :is-scheduled) #f)))
        ((coroutine :is-running)
         (set! (coroutine :please-stop-me) #t))
        (else
         #f)))
        
(define (run-coroutine coroutine args func)
  
  (define (coroutine-helper)
    (set! (coroutine :is-scheduled) #f)
    (when (not (coroutine :please-stop-me))
      (let loop ((args args))
        (set! (coroutine :is-running) #t)
        (let ((pausetime-and-args (catch #t
                                         (lambda ()
                                           (apply func args))
                                         (lambda args
                                           (c-display (ow!))
                                           #f))))
          (set! (coroutine :is-running) #f)
          (if (and (not (coroutine :please-stop-me))
                   pausetime-and-args)
              (let* ((pausetime (car pausetime-and-args))
                     (next-args (cdr pausetime-and-args)))
                (if (> pausetime 0)
                    (schedule-next! next-args pausetime)
                    (loop next-args)))))))
    #f)
  
  (define (schedule-next! next-args pausetime)
    (set! args next-args)
    (set! (coroutine :is-scheduled) #t)
    (<ra> :schedule pausetime coroutine-helper))

  (assert (not (coroutine-alive? coroutine)))
  (set! (coroutine :please-stop-me) #f)
  (set! (coroutine :func) coroutine-helper) ;; Not currently used for anything.
  (schedule-next! args 0))
