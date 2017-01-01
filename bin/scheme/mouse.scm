(provide 'mouse.scm)

(define *left-button* 1)
(define *middle-button* 3)
(define *right-button* 5)

(define (select-button Button)
  (= *left-button* Button))

(define (left-or-right-button Button)
  (or (= *left-button* Button)
      (= *right-button* Button)))


(define (set-statusbar-value val)
  (<ra> :set-statusbar-text (<-> val)))

(define (set-velocity-statusbar-text value)
  (<ra> :set-statusbar-text (<-> "Velocity: " (one-decimal-percentage-string value) "%")))


;; Quantitize
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-match quantitize
  Place Q :> (* (roundup (/ Place Q))
                Q))

#||
(quantitize 18341/2134 1/3)

(begin
  (test (quantitize 0 0.5)
        0.5)
  (test (quantitize 0.5 0.5)
        0.5)
  (test (quantitize 1.0 0.5)
        1.0)
  (test (quantitize 10.0 0.5)
        10)
  (test (quantitize 10.3 0.5)
        10.5)
  (test (quantitize 10.5 0.5)
        10.5)
  (test (quantitize 10.6 0.5)
        10.5)
  (test (quantitize 10.9 0.5)
        11))
||#


;;; Distance
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (square x) (* x x))

#||
;; Todo: use this one instead of the 'get-distance' function below, since this one is much faster. (function originally created for ab, but not used)
(define (distance-to-vector x y vector)
  (define point  (create-point x y))
  (define point1 (vector-point1 vector))
  (define point2 (vector-point2 vector))
  
  (define x1 (point-x point1))
  (define x2 (point-x point2))
  (define y1 (point-y point1))
  (define y2 (point-y point2))
  
  (define distance12 (distance point1 point2))
  (define distance1  (distance point point1))
  (define distance2  (distance point point2))
  
  (if (= 0 distance12)
      distance1
      (begin
        (define distance   (/ (abs (+ (* x  (- y2 y1))
                                      (* (- y) (- x2 x1))
                                      (* y1 x2)
                                      (* (- x1) y2)))
                              distance12))

        (min distance distance1 distance2))))


(define (distance point1 point2)
  (sqrt (+ (square (- (point-x point1)
                      (point-x point2)))
           (square (- (point-y point1)
                      (point-y point2))))))
(define point-x car)
(define point-y cadr)
(define create-point list)
(define vector-point1 car)
(define vector-point2 cadr)
(define (get-distance2 x y x1 y1 x2 y2)
  (distance-to-vector x y (list (create-point x1 y1) (create-point x2 y2))))

||#


;; shortest distance between a point and a non-infinite line. Can be optimized (a lot).
(define (get-distance x y x1 y1 x2 y2)
  (define dist-1-to-2 (sqrt (+ (square (- x2 x1))
                               (square (- y2 y1)))))
  (define dist-0-to-1 (sqrt (+ (square (- x1 x))
                               (square (- y1 y)))))
  (define dist-0-to-2 (sqrt (+ (square (- x2 x))
                               (square (- y2 y)))))                       
  (cond ((= 0 dist-1-to-2)
         dist-0-to-1)
        ((= 0 dist-0-to-1)
         0)
        ((= 0 dist-0-to-2)
         0)
        (else
         (define angle-1 (acos (/ (- (+ (square dist-1-to-2)
                                        (square dist-0-to-1))
                                     (square dist-0-to-2))
                                  (* 2
                                     dist-1-to-2
                                     dist-0-to-1))))
         
         (define angle-2 (acos (/ (- (+ (square dist-1-to-2)
                                        (square dist-0-to-2))
                                     (square dist-0-to-1))
                                  (* 2
                                     dist-1-to-2
                                     dist-0-to-2))))
         ;;(c-display "angle-1" (/ (* 180 angle-1) pi))
         ;;(c-display "angle-2" (/ (* 180 angle-2) pi))
         
         (if (or (>= angle-1 (/ pi 2))
                 (>= angle-2 (/ pi 2)))
             (min dist-0-to-1
                  dist-0-to-2)
             (/ (abs (- (* (- x2 x1) ;; http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
                           (- y1 y))
                        (* (- x1 x)
                           (- y2 y1))))
                dist-1-to-2)))))

(define (get-distance-vertical x y x1 y1 x2 y2)
  (get-distance y x y1 x1 y2 x2))

#||
(test (get-distance 0 0
                    10 0
                    20 0)
      10)

(test (get-distance 30 0
                    10 0
                    20 0)
      10)

(test (get-distance 0 30
                    0 10
                    0 20)
      10)

(test (get-distance 4 10
                    5 0              
                    5 10)
      1)

(test (get-distance 0 0
                    0 0
                    5 5)
      0.0)
(test (get-distance 0 0
                    0 0
                    0 0)
      0.0)
(test (get-distance 0 0
                    5 0
                    5 0)
      5.0)
||#


#||
(define (get-quantitized-place-from-y Button Y)
  (define place (<ra> :get-place-from-y Y))
  (quantitize place (<ra> :get-quantitize)))
||#

(define (get-place-from-y Button Y)
  (if (<ra> :ctrl-pressed)
      (<ra> :get-place-from-y Y)
      (<ra> :get-place-in-grid-from-y Y)))

(define (get-next-place-from-y Button Y)
  (if (<ra> :ctrl-pressed)
      (<ra> :get-place-from-y (+ Y 1))
      (<ra> :get-next-place-in-grid-from-y Y)))


;; Mouse move handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define *mouse-move-handlers* '())

(delafina (add-mouse-move-handler :move)
  (push-back! *mouse-move-handlers* move))


(define (run-mouse-move-handlers button x y)
  (for-each (lambda (move-handler)
              (move-handler button x y))
            *mouse-move-handlers*))


;; Mouse cycles
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define-struct mouse-cycle
  :press-func 
  :drag-func #f
  :release-func #f
  )

(define *mouse-cycles* '())
(define *current-mouse-cycle* #f)

(define (add-mouse-cycle $mouse-cycle)
  (push-back! *mouse-cycles*
              $mouse-cycle))

(define (get-mouse-cycle $button $x $y)
  (find-if (lambda (cycle)
             ((cycle :press-func) $button $x $y))
           *mouse-cycles*))

(define (only-y-direction)
  (<ra> :shift-pressed))

(define (only-x-direction)
  (<ra> :left-extra-pressed))

(delafina (add-delta-mouse-handler :press :move-and-release :release #f :mouse-pointer-is-hidden-func #f)
  (define prev-x #f)
  (define prev-y #f)
  (define value #f)
  
  (define next-mouse-x #f)
  (define next-mouse-y #f)
  
  (define (call-move-and-release $button $x $y)
    ;;(c-display "call-move-and-release" $x $y)
    ;; Ignore all $x and $y values that was already queued when we sat a new mouse pointer position. (needed in qt5)
    (when next-mouse-x
      (if (and (< (abs (- $x next-mouse-x)) 100)  ;; Need some buffer, unfortunately we don't always get the same mouse event back when calling (<ra> :move-mouse-pointer).
               (< (abs (- $y next-mouse-y)) 100)) ;; same here.
          (begin
            (set! prev-x next-mouse-x)
            (set! prev-y next-mouse-y)
            (set! next-mouse-x #f))
          (begin
            (set! $x prev-x)
            (set! $y prev-y))))
            
    (if (and (morally-equal? $x prev-x)
             (morally-equal? $y prev-y)
             (not value))
        value
        (begin
          (define dx (cond ((only-y-direction)
                            0)
                           ((<ra> :ctrl-pressed)
                            (/ (- $x prev-x)
                               10))
                           (else
                            (- $x prev-x))))
          (define dy (cond ((only-x-direction)
                            0)
                           ((<ra> :ctrl-pressed)
                            (/ (- $y prev-y)
                               10))
                           (else
                            (- $y prev-y))))

          (set! prev-x $x)
          (set! prev-y $y)
          
          ;; dirty trick to avoid the screen edges
          ;;
          (when (and mouse-pointer-is-hidden-func (mouse-pointer-is-hidden-func)) ;; <- this line can cause mouse pointer to be stuck between 100,100 and 500,500 if something goes wrong.
            ;;(when mouse-pointer-has-been-set ;; <- Workaround. Hopefully there's no problem doing it like this.
            (when (or (< (<ra> :get-mouse-pointer-x) 100)
                      (< (<ra> :get-mouse-pointer-y) 100)
                      (> (<ra> :get-mouse-pointer-x) 500)
                      (> (<ra> :get-mouse-pointer-y) 500))
              (<ra> :move-mouse-pointer 300 300)
              ;;(c-display "x/y" (<ra> :get-mouse-pointer-x) (<ra> :get-mouse-pointer-y))
              ;;(set! prev-x 300)
              ;;(set! prev-y 300)
              (set! next-mouse-x 300)
              (set! next-mouse-y 300)
              ))
          
          (set! value (move-and-release $button
                                        dx
                                        dy
                                        value)))))
  
  (add-mouse-cycle (make-mouse-cycle
                    :press-func (lambda ($button $x $y)
                                  (set! value (press $button $x $y))
                                  (if value
                                      (begin
                                        (set! prev-x $x)
                                        (set! prev-y $y)
                                        #t)
                                      #f))
                    :drag-func  call-move-and-release
                    :release-func (lambda ($button $x $y)
                                    (call-move-and-release $button $x $y)
                                    (if release
                                        (release $button $x $y value))
                                    (set! prev-x #f)
                                    (set! prev-y #f)))))
  


;; Functions called from radium
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define mouse-fx-has-been-set #f)
(define (set-mouse-fx fxnum tracknum)
  (set! mouse-fx-has-been-set #t)
  (<ra> :set-mouse-fx fxnum tracknum))

(define mouse-track-has-been-set #f)
(define (set-mouse-track tracknum)
  (set! mouse-track-has-been-set #t)
  (<ra> :set-mouse-track tracknum))
(define (set-mouse-track-to-reltempo)
  (set! mouse-track-has-been-set #t)
  (<ra> :set-mouse-track-to-reltempo))

(define mouse-note-has-been-set #f)
(define (set-mouse-note notenum tracknum)
  (set! mouse-note-has-been-set #t)
  (<ra> :set-mouse-note notenum tracknum))

(define indicator-node-has-been-set #f)
(define (set-indicator-temponode num)
  (set! indicator-node-has-been-set #t)
  (<ra> :set-indicator-temponode num))
(define (set-indicator-pitchnum num tracknum)
  (set! indicator-node-has-been-set #t)
  (<ra> :set-indicator-pitchnum num tracknum))
(define (set-indicator-velocity-node velocitynum notenum tracknum)
  (set! indicator-node-has-been-set #t)
  (<ra> :set-indicator-velocity-node velocitynum notenum tracknum))
(define (set-indicator-fxnode fxnodenum notenum tracknum)
  (set! indicator-node-has-been-set #t)
  (<ra> :set-indicator-fxnode fxnodenum notenum tracknum))

(define current-node-has-been-set #f)
(define (set-current-temponode num)
  (set! current-node-has-been-set #t)
  (<ra> :set-current-temponode num))
(define (set-current-velocity-node velnum notenum tracknum)
  (set! current-node-has-been-set #t)
  (set-velocity-statusbar-text (<ra> :get-velocity-value velnum notenum tracknum))
  (<ra> :set-current-velocity-node velnum notenum tracknum))
(define (set-current-fxnode fxnodenum fxnum tracknum)
  (set! current-node-has-been-set #t)
  (<ra> :set-statusbar-text (<ra> :get-fx-string fxnodenum fxnum tracknum))
  (<ra> :set-current-fxnode fxnodenum fxnum tracknum))
(define (set-current-pitchnum pitchnum tracknum)
  (set! current-node-has-been-set #t)
  (<ra> :set-current-pitchnum pitchnum tracknum)
  (<ra> :set-statusbar-text (<-> "Pitch: " (two-decimal-string (<ra> :get-pitchnum-value pitchnum tracknum)))))

(define current-pianonote-has-been-set #f)
(define (set-current-pianonote pianonotenum notenum tracknum)
  (set! current-pianonote-has-been-set #t)
  (<ra> :set-current-pianonote pianonotenum notenum tracknum))
;;  (<ra> :set-statusbar-text (<-> "Pitch: " (two-decimal-string (<ra> :get-pitchnum-value pianonotenum tracknum)))))

(define mouse-pointer-has-been-set #f)
(define (set-mouse-pointer func)
  ;;(c-display "setting to" func)
  (set! mouse-pointer-has-been-set #t)
  (func)
  )

;; TODO: block->is_dirty is set unnecessarily often to true this way.
(define (cancel-current-stuff)
  (<ra> :set-no-mouse-fx)
  (<ra> :set-no-mouse-note)
  (<ra> :set-no-mouse-track)
  (<ra> :cancel-current-node)
  (<ra> :cancel-current-pianonote)
  (<ra> :cancel-indicator-node)
  )

(define (handling-nodes thunk)
  (set! mouse-fx-has-been-set #f)
  (set! mouse-track-has-been-set #f)
  (set! mouse-note-has-been-set #f)
  (set! indicator-node-has-been-set #f)
  (set! current-node-has-been-set #f)
  (set! current-pianonote-has-been-set #f)
  (set! mouse-pointer-has-been-set #f)

  (<ra> :set-statusbar-text "")
  
  (define ret
    (catch #t
           thunk
           (lambda args
             ;; Commenting out output printing here since it causes unnecessary lines to be put into the (ow!) backtrace.
             ;;
             ;;(display "args")(display args)(newline)
             ;;(c-display "Resetting mouse cycle since I caught something:" (car args))
             ;;(apply format (cons '() args)))
             ;;(if (string? (cadr args))
             ;;    (apply format #t (cadr args))
             ;;    (c-display (cadr args)))
             ;;(c-display "    GAKK GAKK GAKK " args)
             (display (ow!))
             ;;
             (set! *current-mouse-cycle* #f)
             (throw (car args)) ;; rethrowing
             #f)))
  
  (if (not mouse-fx-has-been-set)
      (<ra> :set-no-mouse-fx))

  (if (not mouse-track-has-been-set)
      (<ra> :set-no-mouse-track))

  (if (not mouse-note-has-been-set)
      (<ra> :set-no-mouse-note))

  (if (not indicator-node-has-been-set)
      (<ra> :cancel-indicator-node))

  (if (not current-node-has-been-set)
      (<ra> :cancel-current-node))

  (if (not current-pianonote-has-been-set)
      (<ra> :cancel-current-pianonote))

  ;;(if (not mouse-pointer-has-been-set)
  ;;    (<ra> :set-normal-mouse-pointer))

  ret)


(define (radium-mouse-press $button $x $y)
  (handling-nodes
   (lambda()
     ;;(c-display "%%%%%%%%%%%%%%%%% mouse press" $button $x $y *current-mouse-cycle*)
     ;;(cancel-current-stuff)
     (if (not *current-mouse-cycle*)
         (let ((new-mouse-cycle (get-mouse-cycle $button $x $y)))
           (if (and new-mouse-cycle
                    (new-mouse-cycle :drag-func))
               (set! *current-mouse-cycle* new-mouse-cycle))))
     *current-mouse-cycle*)))

(define (radium-mouse-move $button $x $y)
  ;;(c-display "radium-mouse-move" $x $y)

  (handling-nodes
   (lambda()
     ;;(c-display "mouse move2" $button $x $y (<ra> :ctrl-pressed) (<ra> :shift-pressed))
     ;;(cancel-current-stuff)
     (if *current-mouse-cycle*
         (begin 
           ((*current-mouse-cycle* :drag-func) $button $x $y)
           #t)
         (begin
           (run-mouse-move-handlers $button $x $y)
           #f)))))

(define (radium-mouse-release $button $x $y)
  (handling-nodes
   (lambda()
     ;;(c-display "mouse release" $button $x $y)
     (if *current-mouse-cycle*
         (begin
           ((*current-mouse-cycle* :release-func) $button $x $y)
           (set! *current-mouse-cycle* #f)
           (run-mouse-move-handlers $button $x $y)
           (cancel-current-stuff)
           (<ra> :set-normal-mouse-pointer)
           #t)
         #f))))


#||
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda ($button $x $y)
                                (c-display "pressed it" $x $y)
                                #t)
                  :drag-func  (lambda ($button $x $y)
                                (c-display "moved it" $x $y))
                  :release-func (lambda ($button $x $y)
                                  (c-display "released it" $x $y))))
||#


(define-match get-track-num-0
  X _ ___ X1 __ __________ :> #f  :where (< X X1)
  X _ Num X1 X2 __________ :> Num :where (and (>= X X1)
                                              (< X X2))
  _ _ Num __ __ Num-tracks :> #f  :where (= (1+ Num) Num-tracks)
  X Y Num X1 X2 Num-tracks :> (get-track-num-0 X
                                               Y
                                               (1+ Num)
                                               X2
                                               (if (= Num (- Num-tracks 2))
                                                   (<ra> :get-track-x2 (1+ Num))
                                                   (<ra> :get-track-x1 (+ 2 Num)))
                                               Num-tracks))

(define-match get-track-num
  X Y :> (let ((Num-tracks (<ra> :get-num-tracks)))
           (get-track-num-0 X Y (<ra> :get-leftmost-track-num)
                            (<ra> :get-track-x1 (<ra> :get-leftmost-track-num))
                            (<ra> :get-track-x2 (<ra> :get-leftmost-track-num))
                            Num-tracks)))
                                                   
  
#||
(get-track-num 650 50)
||#

(define *current-track-num-all-tracks* #f) ;; Includes the time tracks, linenumbers, and so forth. (see nsmtracker.h)
(define *current-track-num* #f)

(define (set-current-track-num! X Y)
  (define track-num (get-track-num X Y))
  (set! *current-track-num-all-tracks* track-num)
  (if (and track-num
           (>= track-num 0))
      (set! *current-track-num* track-num)
      (set! *current-track-num* #f))
  (cond (*current-track-num*
         (set-mouse-track *current-track-num*))
        ((and (<ra> :reltempo-track-visible)
              *current-track-num-all-tracks*
              (= *current-track-num-all-tracks* (<ra> :get-rel-tempo-track-num)))
         (set-mouse-track-to-reltempo))))

;; Set current track and mouse track
(add-mouse-move-handler
 :move (lambda (Button X Y)
         (set-current-track-num! X Y)))

(define *current-subtrack-num* #f)

(define-match get-subtrack-from-x-0
  __ _ Num Num   ________ :> #f  
  X1 X Num Total Tracknum :> (let ((X2 (if (= Num (1- Total))
                                           (<ra> :get-subtrack-x2 Num Tracknum)                                           
                                           (<ra> :get-subtrack-x1 (1+ Num) Tracknum))))
                               (if (and (>= X X1)
                                        (<  X X2))
                                   Num
                                   (get-subtrack-from-x-0 X2
                                                          X
                                                          (1+ Num)
                                                          Total
                                                          Tracknum))))

(define-match get-subtrack-from-x
  X Tracknum :> (get-subtrack-from-x-0 (<ra> :get-subtrack-x1 0 Tracknum)
                                       X
                                       0
                                       (<ra> :get-num-subtracks Tracknum) Tracknum))

(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (set! *current-subtrack-num* (and *current-track-num*
                                           (inside-box (<ra> :get-box track-fx *current-track-num*) $x $y)
                                           (get-subtrack-from-x $x *current-track-num*)))))


;;;;;;;;;;;;;;;;;;;;;;

#||
(<ra> :set-reltempo 0.2)
||#

(define (get-common-node-box $x $y)
  (define width/2 (<ra> :get-half-of-node-width))
  (define x1 (- $x width/2))
  (define y1 (- $y width/2))
  (define x2 (+ $x width/2))
  (define y2 (+ $y width/2))
  (make-box2 x1 y1 x2 y2))
  

(delafina (find-node :$x
                     :$y
                     :$get-node-box
                     :$num-nodes
                     :$num 0
                     )

  (cond ((= 0 $num-nodes)
         (list 'new-box 0))
        (else
         (define box ($get-node-box $num))
         (cond ((inside-box box $x $y)
                (list 'existing-box $num box))
               ((and (= 0 $num)
                     (< $y (box :y1)))
                'before)
               ((> (box :y1) $y)
                (list 'new-box $num))
               ((= $num (1- $num-nodes))
                'after)
               (else
                (find-node $x $y $get-node-box $num-nodes (1+ $num)))))))

(delafina (find-node-horizontal :$x
                                :$y
                                :$get-node-box
                                :$num-nodes
                                :$num 0
                                )
  
  (cond ((= 0 $num-nodes)
         (list 'new-box 0))
        (else
         (define box ($get-node-box $num))
         (cond ((inside-box box $x $y)
                (list 'existing-box $num box))
               ((and (= 0 $num)
                     (< $x (box :x1)))
                'before)
               ((> (box :x1) $x)
                (list 'new-box $num))
               ((= $num (1- $num-nodes))
                'after)
               (else
                (find-node-horizontal $x $y $get-node-box $num-nodes (1+ $num)))))))


(define-struct move-node-handler
  :move
  :release
  :node
  )

(define-struct node-mouse-cycle
  :press
  :move-and-release
  :release
  )

(define *move-existing-node-mouse-cycles* '())
(define *create-new-node-mouse-cycles* '())

(define-match get-cycle-and-node
  ______ _ _ ()             :> #f
  Button X Y (Cycle . Rest) :> (let ((Node ((Cycle :press) Button X Y)))
                                 (if Node
                                     (make-move-node-handler :move (Cycle :move-and-release)
                                                             :release (Cycle :release)
                                                             :node Node)
                                     (get-cycle-and-node Button X Y Rest))))



(define *mouse-pointer-is-currently-hidden* #t)

;; This cycle handler makes sure all move-existing-node cycles are given a chance to run before the create-new-node cycles.
(add-delta-mouse-handler
 :press (lambda (Button X Y)
          (get-cycle-and-node Button
                              X
                              Y
                              (append *move-existing-node-mouse-cycles*
                                      *create-new-node-mouse-cycles*)))
 
 :move-and-release (lambda (Button Dx Dy Cycle-and-node)
                     (define Move (Cycle-and-node :move))
                     (define Release (Cycle-and-node :release))
                     (define Old-node (Cycle-and-node :node))
                     (define New-node (Move Button Dx Dy Old-node))
                     (make-move-node-handler :move Move
                                             :release Release
                                             :node New-node))
 
 :release (lambda (Button X Y Cycle-and-node)
            (define Release (Cycle-and-node :release))
            (define node (Cycle-and-node :node))
            (Release Button X Y node))
 
 :mouse-pointer-is-hidden-func (lambda () *mouse-pointer-is-currently-hidden*)
 ) 

   
(delafina (add-node-mouse-handler :Get-area-box
                                  :Get-existing-node-info
                                  :Get-min-value
                                  :Get-max-value
                                  :Get-x #f ;; Only used when releasing mouse button
                                  :Get-y #f ;; Only used when releasing mouse button
                                  :Make-undo 
                                  :Create-new-node
                                  :Move-node
                                  :Release-node #f
                                  :Publicize
                                  :Get-pixels-per-value-unit #f
                                  :Create-button #f
                                  :Use-Place #t
                                  :Mouse-pointer-func #f
                                  )
  
  (define-struct node
    :node-info
    :value    
    :y)

  (define (press-existing-node Button X Y)
    (and (select-button Button)
         (let ((area-box (Get-area-box)))
           ;;(c-display X Y "area-box" (and area-box (box-to-string area-box)) (and area-box (inside-box-forgiving area-box X Y)) (box-to-string (<ra> :get-box reltempo-slider)))
           (and area-box
                (inside-box-forgiving area-box X Y)))
         (Get-existing-node-info X
                                 Y
                                 (lambda (Node-info Value Node-y)
                                   (Make-undo Node-info)
                                   (Publicize Node-info)
                                   (if Mouse-pointer-func
                                       (set! *mouse-pointer-is-currently-hidden* #f)
                                       (set! *mouse-pointer-is-currently-hidden* #t))
                                   (set-mouse-pointer (or Mouse-pointer-func ra:set-blank-mouse-pointer))
                                   (make-node :node-info Node-info
                                              :value Value
                                              :y Node-y
                                              )))))

  (define (can-create Button X Y)
    (and (or (and Create-button (= Button Create-button))
             (and (not Create-button) (select-button Button)))
         (let ((area-box (Get-area-box)))
           (and area-box
                (inside-box area-box X Y)))))
    
  (define (press-and-create-new-node Button X Y)
    (and (can-create Button X Y)
         (Create-new-node X
                          (if Use-Place
                              (get-place-from-y Button Y)
                              Y)
                          (lambda (Node-info Value)
                            (Publicize Node-info)
                            (if Mouse-pointer-func
                                (set! *mouse-pointer-is-currently-hidden* #f)
                                (set! *mouse-pointer-is-currently-hidden* #t))
                            (set-mouse-pointer (or Mouse-pointer-func ra:set-blank-mouse-pointer))
                            (make-node :node-info Node-info
                                       :value Value
                                       :y Y)))))

  (define (move-or-release Button Dx Dy Node)
    (define node-info (Node :node-info))
    (define min (Get-min-value node-info))
    (define max (Get-max-value node-info))
    (define area-box (Get-area-box))
    (define node-area-width (area-box :width))
    (define pixels-per-value-unit (if Get-pixels-per-value-unit
                                      (Get-pixels-per-value-unit node-info)
                                      (/ node-area-width
                                         (- max min))))
    (define new-value (let ((try-it (+ (Node :value)
                                       (/ Dx
                                          pixels-per-value-unit))))
                        (between min try-it max)))

    ;;(c-display "Dy/Y" Dy (Node :y))
    ;;(c-display "num" ($node :num) ($get-num-nodes-func) "value" $dx ($node :value) (node-area :x1) (node-area :x2) ($get-node-value-func ($node :num)))
    (define new-y (if Use-Place
                      (and (not (= 0 Dy))                           
                           (let ((try-it (+ (Node :y)
                                            Dy)))
                             (between (1- (<ra> :get-top-visible-y))
                                      try-it
                                      (+ 2 (<ra> :get-bot-visible-y)))))
                      (+ Dy (Node :y))))

    (define same-pos (and (morally-equal? new-y (Node :y))
                          (morally-equal? new-value (Node :value))))
    
    ;;(c-display "same-pos" same-pos new-y (Node :y) new-value (Node :value))

    (if same-pos
        Node
        (let ((node-info (Move-node node-info new-value
                                    (if (not Use-Place)
                                        new-y
                                        (and new-y
                                             (get-place-from-y Button new-y))))))
          (Publicize node-info)
          (make-node :node-info node-info
                     :value new-value
                     :y (or new-y (Node :y))))))
  
  (define (move-and-release Button Dx Dy Node)
    (move-or-release Button Dx Dy Node))
  
  (define (release Button Dx Dy Node)
    (define node-info (Node :node-info))
    (if Release-node
        (Release-node node-info))
    (if (and Get-x Get-y)
        (let ((x (Get-x node-info))
              (y (Get-y node-info)))
          (and x y
               (<ra> :move-mouse-pointer x y)))))

  (define move-existing-node-mouse-cycle (make-node-mouse-cycle :press press-existing-node
                                                                :move-and-release move-and-release
                                                                :release release))
  
  (define create-new-node-mouse-cycle (make-node-mouse-cycle :press press-and-create-new-node
                                                             :move-and-release move-and-release
                                                             :release release))

  (push-back! *move-existing-node-mouse-cycles* move-existing-node-mouse-cycle)
  (push-back! *create-new-node-mouse-cycles* create-new-node-mouse-cycle)

  )

      

;; Used for sliders and track width
(delafina (add-horizontal-handler :Get-handler-data
                                  :Get-x1
                                  :Get-x2
                                  :Get-min-value
                                  :Get-max-value
                                  :Get-x #f
                                  :Get-value
                                  :Make-undo
                                  :Release #f
                                  :Move
                                  :Publicize
                                  :Mouse-pointer-func #f)
 
  (define-struct info
    :handler-data
    :y)

  (add-node-mouse-handler :Get-area-box (lambda () (make-box2 0 0 100000 100000))
                          :Get-existing-node-info (lambda (X Y callback)
                                                    ;;(c-display "  horiz: " X Y)
                                                    (define handler-data (Get-handler-data X Y))
                                                    (and handler-data
                                                         (let ((info (make-info :handler-data handler-data
                                                                                :y Y)))
                                                           (callback info
                                                                     (Get-value handler-data)
                                                                     0))))
                          :Get-min-value (lambda (Info)
                                           (Get-min-value (Info :handler-data)))
                          :Get-max-value (lambda (Info)
                                           (Get-max-value (Info :handler-data)))
                          :Get-x (lambda (Info)
                                   (and Get-x
                                        (Get-x (Info :handler-data))))
                          :Get-y (lambda (Info)
                                   (and Get-x
                                        (Info :y)))
                          :Make-undo (lambda (Info)
                                       (Make-undo (Info :handler-data)))
                          :Create-new-node (lambda (Value Place callback)
                                             #f)
                          :Release-node (lambda x
                                          (if Release
                                              (Release)))
                          :Move-node (lambda (Info Value Place)
                                       (Move (Info :handler-data)
                                             Value)
                                       Info)
                          :Publicize (lambda (Info)
                                       (Publicize (Info :handler-data)))
                          :Get-pixels-per-value-unit (lambda (Info)
                                                       (/ (- (Get-x2 (Info :handler-data))
                                                             (Get-x1 (Info :handler-data)))
                                                          (- (Get-max-value (Info :handler-data))
                                                             (Get-min-value (Info :handler-data)))))
                          :Mouse-pointer-func Mouse-pointer-func
                          ))
                                  



;; status bar and mouse pointer for block and track sliders and track on/off buttons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(add-mouse-move-handler
 :move (lambda ($button X Y)
         ;;(c-display X Y (box-to-string (get-seqnav-move-box)))
         (cond ((and *current-track-num*
                     (inside-box (<ra> :get-box track-pan-slider *current-track-num*) X Y))
                (set-mouse-pointer ra:set-horizontal-split-mouse-pointer)
                (show-track-pan-in-statusbar *current-track-num*))
               
               ((and *current-track-num*
                     (inside-box (<ra> :get-box track-volume-slider *current-track-num*) X Y))
                (set-mouse-pointer ra:set-horizontal-resize-mouse-pointer)
                (show-track-volume-in-statusbar *current-track-num*))
               
               ((inside-box (<ra> :get-box reltempo-slider) X Y)
                (set-mouse-pointer ra:set-horizontal-resize-mouse-pointer)
                (show-reltempo-in-statusbar))
               
               ((and *current-track-num*
                     (inside-box (<ra> :get-box track-pan-on-off *current-track-num*) X Y))
                (set-mouse-pointer ra:set-pointing-mouse-pointer)
                (<ra> :set-statusbar-text (<-> "Track panning slider " (if (<ra> :get-track-pan-on-off *current-track-num*) "on" "off"))))
               
               ((and *current-track-num*
                     (inside-box (<ra> :get-box track-volume-on-off *current-track-num*) X Y))
                (set-mouse-pointer ra:set-pointing-mouse-pointer)
                (<ra> :set-statusbar-text (<-> "Track volume slider " (if (<ra> :get-track-volume-on-off *current-track-num*) "on" "off"))))

               ((and *current-track-num*
                     (< Y (<ra> :get-track-pan-on-off-y1)))
                (set-mouse-pointer ra:set-pointing-mouse-pointer)
                (<ra> :set-statusbar-text (<-> "Select instrument for track " *current-track-num*)))

               ((inside-box (<ra> :get-box sequencer) X Y)
                (cond ((get-seqblock-info X Y)
                       (set-mouse-pointer ra:set-open-hand-mouse-pointer))
                      ((inside-box (get-seqnav-move-box) X Y)
                       (set-mouse-pointer ra:set-open-hand-mouse-pointer))
                      ((inside-box (<ra> :get-box seqnav-left-size-handle) X Y)
                       (set-mouse-pointer ra:set-horizontal-resize-mouse-pointer))
                      ((inside-box (<ra> :get-box seqnav-right-size-handle) X Y)
                       (set-mouse-pointer ra:set-horizontal-resize-mouse-pointer))
                      (else
                       (<ra> :set-normal-mouse-pointer))))
               
               ((not *current-track-num*)
                (set-mouse-pointer ra:set-pointing-mouse-pointer))
               
               (else
                ;;(<ra> :set-normal-mouse-pointer)
                ))))



;; block tempo multiplier slider
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (show-reltempo-in-statusbar)
  (<ra> :set-statusbar-text (<-> "Block tempo multiplied by " (two-decimal-string (<ra> :get-reltempo)))))


(define (get-BPMs)
  (map (lambda (bpmnum)
         (list (<ra> :get-bpm-place bpmnum)
               (<ra> :get-bpm bpmnum)))
       (iota (<ra> :num-bpms))))

#!!
(get-BPMs)
!!#

(define (get-temponodes)
  (map (lambda (num)
         (list (<ra> :get-temponode-place num)
               (<ra> :get-temponode-value num)))
       (iota (<ra> :get-num-temponodes))))
#!!
(get-temponodes)
!!#


(define (reset-tempo-multiplier)
  (<ra> :undo-reltempo)
  (<ra> :set-reltempo 1.0))

(define (apply-tempo-multiplier-to-block)
  (undo-block
   (lambda ()
     (let* ((reltempo (<ra> :get-reltempo))
            (bpms (get-BPMs))
            (scale-bpm (lambda (bpm)
                         (round (* reltempo bpm)))))
       (for-each (lambda (place-and-bpm)
                   (let ((place (car place-and-bpm))
                         (bpm (cadr place-and-bpm)))
                     (<ra> :add-bpm (scale-bpm bpm) place)))
                 bpms)
       (if (or (null? bpms)
               (> (car (car bpms)) 0))
           (<ra> :add-bpm (scale-bpm (<ra> :get-main-bpm)) 0))
       (reset-tempo-multiplier)))))

(define (apply-bpm-glide bpmnum)
  (undo-block
   (lambda ()
     (define bpms (get-BPMs))
     (define bpm1 (list-ref bpms bpmnum))
     (define bpm2 (list-ref bpms (1+ bpmnum)))
     (define temponodes (get-temponodes))
     (set! temponodes (nodelist-add-same-value-at-place (car bpm1) 0))
     (set! temponodes (nodelist-add-same-value-at-place (car bpm2) 0))
     (set! temponodes (nodelist-mix temponodes (list (create-node (car bpm1) 0)
                                                     (create-node (-line (car bpm2)) 2)))) ;; Fix 2.
     )))


(define (get-reltemposlider-x)
  (define box (<ra> :get-box reltempo-slider))
  (scale (<ra> :get-reltempo)
         (<ra> :get-min-reltempo)
         (<ra> :get-max-reltempo)
         (box :x1)
         (box :x2)))

;; slider
(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (define box (<ra> :get-box reltempo-slider))
                                            (and (inside-box box X Y)
                                                 (<ra> :get-reltempo)))
                        :Get-x1 (lambda (_)
                                  (<ra> :get-reltempo-slider-x1))
                        :Get-x2 (lambda (_)
                                  (<ra> :get-reltempo-slider-x2))
                        :Get-min-value (lambda (_)
                                         (<ra> :get-min-reltempo))
                        :Get-max-value (lambda (_)
                                         (<ra> :get-max-reltempo))
                        :Get-x (lambda (_)
                                 (get-reltemposlider-x))
                        :Get-value (lambda (Value)
                                     Value)
                        :Make-undo (lambda (_)
                                     (<ra> :undo-reltempo))
                        :Move (lambda (_ Value)
                                (<ra> :set-reltempo Value))
                        :Publicize (lambda (_)
                                     (show-reltempo-in-statusbar))
                        )

;; reset slider value
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)                                
                                (if (and (= Button *right-button*)
                                         (inside-box (<ra> :get-box reltempo-slider) X Y))
                                    (begin
                                      (popup-menu "Reset" reset-tempo-multiplier
                                                  "Apply tempo" apply-tempo-multiplier-to-block)
                                      #t)
                                    #f))))

;; track slider
#||
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)                                
                                (if (inside-box (<ra> :get-box track-slider) X Y)
                                    (begin
                                      (<ra> :show-message "The track slider can not be moved.\nOnly keyboard is supported to navigate to other tracks.")
                                      #t)
                                    #f))))
||#

(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (define box (<ra> :get-box track-slider))
                                            (and (inside-box box X Y)
                                                 (<ra> :current-track)))
                        :Get-x1 (lambda (_)
                                  (<ra> :get-track-slider-x1))
                        :Get-x2 (lambda (_)
                                  (<ra> :get-track-slider-x2))
                        :Get-min-value (lambda (_)
                                         0)
                        :Get-max-value (lambda (_)
                                         (<ra> :get-num-tracks))
                        :Get-x (lambda (_)
                                 (/ (+ (<ra> :get-track-slider-x1)
                                       (<ra> :get-track-slider-x2))
                                    2))
                        :Get-value (lambda (Value)
                                     Value)
                        :Make-undo (lambda (_)
                                     50)
                        :Move (lambda (_ Value)
                                (cond ((>= Value (1+ (<ra> :current-track)))
                                       (<ra> :cursor-right))
                                      ((<= Value (1- (<ra> :current-track)))
                                       (<ra> :cursor-left)))
                                Value)
                        :Publicize (lambda (_)
                                     60)
                        )

(define (show-instrument-color-dialog instrument-id)
  (<ra> :color-dialog (<ra> :get-instrument-color instrument-id)
        (lambda (color)
          (<ra> :set-instrument-color color instrument-id))))

(define (track-configuration-popup X Y)
  (c-display "TRACK " *current-track-num*)
  (popup-menu "Pianoroll     (left alt + p)" :check (<ra> :pianoroll-visible *current-track-num*) (lambda (onoff)
                                                                                                    (<ra> :show-pianoroll onoff *current-track-num*))
              "Note text     (left alt + n)" :check (<ra> :note-track-visible *current-track-num*) (lambda (onoff)
                                                                                                     (<ra> :show-note-track onoff *current-track-num*))
              (list "Cents"
                    :check (<ra> :centtext-visible *current-track-num*)
                    :enabled (<ra> :centtext-can-be-turned-off *current-track-num*)
                    (lambda (onoff)
                      (<ra> :show-centtext  onoff *current-track-num*)))
              "Chance text" :check (<ra> :chancetext-visible *current-track-num*) (lambda (onoff)
                                                                                    (<ra> :show-chancetext onoff *current-track-num*))
              "Velocity text (left alt + y)" :check (<ra> :veltext-visible *current-track-num*) (lambda (onoff)
                                                                                                  (<ra> :show-veltext onoff *current-track-num*))
              "FX text"                      :check (<ra> :fxtext-visible *current-track-num*)   (lambda (onoff)
                                                                                                   (<ra> :show-fxtext onoff *current-track-num*))
              "-------"
              "Copy Track     (left alt + c)" (lambda ()
                                                (<ra> :copy-track *current-track-num*))
              "Cut Track      (left alt + x)"     (lambda ()
                                                    (<ra> :cut-track *current-track-num*))
              "Paste Track    (left alt + v)" (lambda ()
                                                (<ra> :paste-track *current-track-num*))
              "-------"
              "Insert Track     (left alt + i)" (lambda ()
                                                  (<ra> :insert-track *current-track-num*)
                                                  (set-current-track-num! X Y))
              "Delete Track     (left alt + r)" (lambda ()
                                                  (<ra> :delete-track *current-track-num*)
                                                  (set-current-track-num! X Y))
              "-------"
              "Set Instrument     (F12)" (lambda ()
                                           (select-track-instrument *current-track-num*))
              (<-> "Set MIDI channel (now: " (1+ (<ra> :get-track-midi-channel *current-track-num*)) ")")
              (lambda ()
                (c-display "CURETNTE TRSCKN NUM: " *current-track-num*)
                (define channelnum (<ra> :request-integer "MIDI channel (1-16):" 1 16))
                (if (>= channelnum 1)
                    (<ra> :set-track-midi-channel (1- channelnum) *current-track-num*)))
              (let ((instrument-id (<ra> :get-instrument-for-track  *current-track-num*)))
                (list "Configure instrument color"
                      :enabled (>= instrument-id 0)
                      (lambda ()
                        (show-instrument-color-dialog instrument-id))))
              "-------"
              "Help Chance text" (lambda ()
                                   (<ra> :show-chance-help-window))
              "Help Velocity text" (lambda ()
                                     (<ra> :show-velocity-help-window))
              "Help FX text" (lambda ()
                               (<ra> :show-fx-help-window))
              ))

#||        
  (popup-menu "Show/hide Velocity text" (lambda ()
                                          (<ra> :show-hide-veltext *current-track-num*))
              "Show/hide Pianoroll"     (lambda ()
                                          (<ra> :show-hide-pianoroll *current-track-num*))
              "Show/hide Notes"         (lambda ()
                                          (<ra> :show-hide-note-track *current-track-num*))
              ))
||#


;; select instrument for track
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)
                                (cond ((and *current-track-num*
                                            (>= X (<ra> :get-track-x1 0))
                                            (< Y (<ra> :get-track-pan-on-off-y1)))
                                       (if (= Button *right-button*)
                                           (if (<ra> :shift-pressed)
                                               (<ra> :delete-track *current-track-num*)
                                               (track-configuration-popup X Y))
                                           (select-track-instrument *current-track-num*))
                                       #t)
                                      (else
                                       #f)))))


;; track pan on/off
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)
                                (cond ((and *current-track-num*
                                            (inside-box (<ra> :get-box track-pan-on-off *current-track-num*) X Y))
                                       (<ra> :undo-track-pan *current-track-num*)
                                       (<ra> :set-track-pan-on-off (not (<ra> :get-track-pan-on-off *current-track-num*))
                                                                *current-track-num*)
                                       #t)
                                      (else
                                       #f)))))


;; track volume on/off
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)
                                (cond ((and *current-track-num*
                                            (inside-box (<ra> :get-box track-volume-on-off *current-track-num*) X Y))
                                       (<ra> :undo-track-volume *current-track-num*)
                                       (<ra> :set-track-volume-on-off (not (<ra> :get-track-volume-on-off *current-track-num*))
                                                                   *current-track-num*)
                                       #t)
                                      (else
                                       #f)))))




;; track pan sliders
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (show-track-pan-in-statusbar Tracknum)
  (<ra> :set-statusbar-text (<-> "Track pan " (two-decimal-string (<ra> :get-track-pan Tracknum)))))

(define (get-trackpan-x Tracknum)
  (scale (<ra> :get-track-pan Tracknum)
         -1
         1
         (<ra> :get-track-pan-slider-x1 Tracknum)
         (<ra> :get-track-pan-slider-x2 Tracknum)))

;; slider
(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (and *current-track-num*
                                                 (inside-box (<ra> :get-box track-pan-slider *current-track-num*) X Y)
                                                 *current-track-num*))
                        :Get-x1 ra:get-track-pan-slider-x1
                        :Get-x2 ra:get-track-pan-slider-x2
                        :Get-min-value (lambda (_)
                                         -1.0)
                        :Get-max-value (lambda (_)
                                         1.0)
                        :Get-x (lambda (Tracknum)
                                 (get-trackpan-x Tracknum))
                        :Get-value ra:get-track-pan
                        :Make-undo ra:undo-track-pan
                        :Move (lambda (Tracknum Value)
                                ;;(c-display Tracknum Value)
                                (<ra> :set-track-pan Value Tracknum))
                        :Publicize (lambda (Tracknum)
                                     (show-track-pan-in-statusbar Tracknum))
                        )

;; reset slider value
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)
                                (cond ((and *current-track-num*
                                            (inside-box (<ra> :get-box track-pan-slider *current-track-num*) X Y))
                                       (<ra> :undo-track-pan *current-track-num*)
                                       (<ra> :set-track-pan 0.0 *current-track-num*)
                                       #t)
                                      (else
                                       #f)))))

     

;; track volume sliders
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (show-track-volume-in-statusbar Tracknum)
  (<ra> :set-statusbar-text (<-> "Track volume " (two-decimal-string (<ra> :get-track-volume Tracknum)))))

(define (get-trackvolume-x Tracknum)
  (scale (<ra> :get-track-volume Tracknum)
         0
         1
         (<ra> :get-track-volume-slider-x1 Tracknum)
         (<ra> :get-track-volume-slider-x2 Tracknum)))

;; slider
(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (and *current-track-num*
                                                 (inside-box (<ra> :get-box track-volume-slider *current-track-num*) X Y)
                                                 *current-track-num*))
                        :Get-x1 ra:get-track-volume-slider-x1
                        :Get-x2 ra:get-track-volume-slider-x2
                        :Get-min-value (lambda (_)
                                         0.0)
                        :Get-max-value (lambda (_)
                                         1.0)
                        :Get-x (lambda (Tracknum)
                                 (get-trackvolume-x Tracknum))
                        :Get-value ra:get-track-volume
                        :Make-undo ra:undo-track-volume
                        :Move (lambda (Tracknum Value)
                                ;;(c-display Tracknum Value)
                                (<ra> :set-track-volume Value Tracknum))
                        :Publicize (lambda (Tracknum)
                                     (show-track-volume-in-statusbar Tracknum))
                        )


;; reset slider value
(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)
                                (cond ((and *current-track-num*
                                            (inside-box (<ra> :get-box track-volume-slider *current-track-num*) X Y))
                                       (<ra> :undo-track-volume *current-track-num*)
                                       (<ra> :set-track-volume 0.8 *current-track-num*)
                                       #t)
                                      (else
                                       #f)))))

     

;; temponodes
;;;;;;;;;;;;;;;;;;;;;;;;;;;

#||
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (if (inside-box (<ra> :get-box temponode-area) $x $y)
             (c-display "inside" $x $y))))
||#

(define (show-temponode-in-statusbar value)
  (define actual-value (if (< value 0) ;; see reltempo.c
                           (/ 1
                              (- 1 value))
                           (1+ value)))
  (<ra> :set-statusbar-text (<-> "Tempo multiplied by " (two-decimal-string actual-value))))

(define (get-temponode-box $num)
  (get-common-node-box (<ra> :get-temponode-x $num)
                       (<ra> :get-temponode-y $num)))

(define (temponodeval->01 value)
  (scale value
         (- (1- (<ra> :get-temponode-max)))
         (1- (<ra> :get-temponode-max))
         0
         1))

(define (01->temponodeval O1)
  (scale O1
         0
         1
         (- (1- (<ra> :get-temponode-max)))
         (1- (<ra> :get-temponode-max))))
         
(add-node-mouse-handler :Get-area-box (lambda () (and (<ra> :reltempo-track-visible) (<ra> :get-box temponode-area)))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  (and (<ra> :reltempo-track-visible)
                                                       (inside-box-forgiving (<ra> :get-box temponode-area) X Y)
                                                       (match (list (find-node X Y get-temponode-box (<ra> :get-num-temponodes)))
                                                              (existing-box Num Box) :> (callback Num (temponodeval->01 (<ra> :get-temponode-value Num)) (Box :y))
                                                              _                      :> #f)))
                        :Get-min-value (lambda (_) 0);(- (1- (<ra> :get-temponode-max))))
                        :Get-max-value (lambda (_) 1);(1- (<ra> :get-temponode-max)))
                        :Get-x (lambda (Num) (<ra> :get-temponode-x Num))
                        :Get-y (lambda (Num) (<ra> :get-temponode-y Num))
                        :Make-undo (lambda (_) (ra:undo-temponodes))
                        :Create-new-node (lambda (X Place callback)
                                           (define Value (scale X (<ra> :get-temponode-area-x1) (<ra> :get-temponode-area-x2) 0 1))
                                           (define Num (<ra> :add-temponode (01->temponodeval Value) Place))
                                           (if (= -1 Num)
                                               #f
                                               (callback Num (temponodeval->01 (<ra> :get-temponode-value Num)))))
                        :Move-node (lambda (Num Value Place)
                                     (<ra> :set-temponode Num (01->temponodeval Value) (or Place -1))
                                     (define new-value (<ra> :get-temponode-value Num)) ;; might differ from Value
                                     ;;(c-display "Place/New:" Place (<ra> :get-temponode-value Num))
                                     (temponodeval->01 new-value)
                                     Num
                                     )
                        :Publicize (lambda (Num) ;; this version works though. They are, or at least, should be, 100% functionally similar.
                                     (set-indicator-temponode Num)
                                     (show-temponode-in-statusbar (<ra> :get-temponode-value Num)))
                        :Get-pixels-per-value-unit #f
                        )                        

;; delete temponode
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     (<ra> :reltempo-track-visible)
                     (inside-box (<ra> :get-box temponode-area) $x $y)                                     
                     (match (list (find-node $x $y get-temponode-box (<ra> :get-num-temponodes)))
                            (existing-box Num Box) :> (begin
                                                        (<ra> :undo-temponodes)
                                                        (<ra> :delete-temponode Num)
                                                        #t)
                            _                      :> #f)))))

;; highlight current temponode
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (and (<ra> :reltempo-track-visible)
              (inside-box-forgiving (<ra> :get-box temponode-area) $x $y)
              (match (list (find-node $x $y get-temponode-box (<ra> :get-num-temponodes)))
                     (existing-box Num Box) :> (begin
                                                 (set-mouse-track-to-reltempo)
                                                 (set-current-temponode Num)
                                                 (set-indicator-temponode Num)
                                                 (show-temponode-in-statusbar (<ra> :get-temponode-value Num))
                                                 #t)
                     _                      :> #f))))



;; notes
;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define (place-is-last-place place)
  (= (-line (<ra> :get-num-lines))
     place))

(define (note-spans-last-place notenum tracknum)
  (define num-nodes (<ra> :get-num-velocities notenum tracknum))
  (place-is-last-place (<ra> :get-velocity-place
                             (1- num-nodes)
                             notenum
                             tracknum)))

;; pitches
;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (get-pitchnum-box $num)
  ;;(c-display "get-pitchnum-box" $num)
  (make-box2 (<ra> :get-pitchnum-x1 $num *current-track-num*)
             (<ra> :get-pitchnum-y1 $num *current-track-num*)
             (<ra> :get-pitchnum-x2 $num *current-track-num*)
             (<ra> :get-pitchnum-y2 $num *current-track-num*)))

(define (todofunc funcname . $returnvalue)
  (lambda x
    (c-display "\"" funcname "\" not implemented. Arguments: " x)
    (if (null? $returnvalue)
        'no-return-value
        (car $returnvalue))))
  
#||
(set! *current-track-num* 0)
(box-to-string (get-pitchnum-box 1))
(<ra> :get-num-pitchnum 0)
(<ra> :get-pitchnum-value 1 0)
||#

#||
(add-delta-mouse-handler
 :press (lambda ($button $x $y)
          (c-display $x $y)
          #f))
||#


(define-match get-min-pitch-in-current-track-0
  N N   #f           :> 0
  N N   Least-So-Far :> Least-So-Far
  N Max #f           :> (get-min-pitch-in-current-track-0 (1+ N)
                                                          Max
                                                          (<ra> :get-pitchnum-value N *current-track-num*))
  N Max Least-So-Far :> (get-min-pitch-in-current-track-0 (1+ N)
                                                          Max
                                                          (min Least-So-Far
                                                               (<ra> :get-pitchnum-value N *current-track-num*))))
  
(define (get-min-pitch-in-current-track)
  (1- (get-min-pitch-in-current-track-0 0
                                        (<ra> :get-num-pitchnums *current-track-num*)
                                        #f)))
       
(define-match get-max-pitch-in-current-track-0
  N N   #f           :> 127
  N N   Least-So-Far :> Least-So-Far
  N Max #f           :> (get-max-pitch-in-current-track-0 (1+ N)
                                                          Max
                                                          (<ra> :get-pitchnum-value N *current-track-num*))
  N Max Least-So-Far :> (get-max-pitch-in-current-track-0 (1+ N)
                                                          Max
                                                          (max Least-So-Far
                                                               (<ra> :get-pitchnum-value N *current-track-num*))))
  
(define (get-max-pitch-in-current-track)
  (1+ (get-max-pitch-in-current-track-0 0
                                        (<ra> :get-num-pitchnums *current-track-num*)
                                        #f)))

;; add and move pitch
(add-node-mouse-handler :Get-area-box (lambda ()
                                        (and *current-track-num*
                                             (<ra> :get-box track-notes *current-track-num*)))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  '(c-display "hepp"
                                                              (may-be-a-resize-point-in-track X Y *current-track-num*)
                                                              (list (find-node X Y get-pitchnum-box (<ra> :get-num-pitchnums *current-track-num*))))
                                                  (and *current-track-num*
                                                       (not (may-be-a-resize-point-in-track X Y *current-track-num*))
                                                       (match (list (find-node X Y get-pitchnum-box (<ra> :get-num-pitchnums *current-track-num*)))
                                                              (existing-box Num Box) :> (callback Num (<ra> :get-pitchnum-value Num *current-track-num*) (Box :y))
                                                              _                      :> #f)))
                        :Get-min-value (lambda (_)
                                         (get-min-pitch-in-current-track))
                        :Get-max-value (lambda (_)
                                         (get-max-pitch-in-current-track))
                        :Get-x (lambda (Num)
                                 ;;(c-display "    NUM----> " Num)
                                 (<ra> :get-pitchnum-x Num *current-track-num*))
                        :Get-y (lambda (Num)
                                 (<ra> :get-pitchnum-y Num *current-track-num*))
                        :Make-undo (lambda (_) (<ra> :undo-notes *current-track-num*))
                        :Create-new-node (lambda (X Place callback)
                                           (if (place-is-last-place Place)
                                               #f
                                               (begin
                                                 (define Value (scale X
                                                                      (<ra> :get-track-notes-x1 *current-track-num*) (<ra> :get-track-notes-x2 *current-track-num*) 
                                                                      (get-min-pitch-in-current-track) (get-max-pitch-in-current-track)))
                                                 (if (not (<ra> :ctrl-pressed))
                                                     (set! Value (round Value)))
                                                 (define Num (<ra> :add-pitchnum Value Place *current-track-num*))
                                                 (if (= -1 Num)
                                                     #f
                                                     (callback Num (<ra> :get-pitchnum-value Num *current-track-num*))))))
                        :Move-node (lambda (Num Value Place)                                     
                                     (<ra> :set-pitchnum Num
                                                        (if (<ra> :ctrl-pressed)
                                                            Value
                                                            (round Value))
                                                        (or Place -1)
                                                        *current-track-num*))
                        :Publicize (lambda (Num)
                                     (set-indicator-pitchnum Num *current-track-num*)
                                     (<ra> :set-statusbar-text (<-> "Pitch: " (two-decimal-string (<ra> :get-pitchnum-value Num *current-track-num*)))))
                        :Get-pixels-per-value-unit (lambda (_)
                                                     5.0)
                        )


;; delete pitch
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     (<ra> :shift-pressed)
                     *current-track-num*
                     (inside-box (<ra> :get-box track-notes *current-track-num*) $x $y)
                     (match (list (find-node $x $y get-pitchnum-box (<ra> :get-num-pitchnums *current-track-num*)))
                            (existing-box Num Box) :> (begin
                                                        (<ra> :undo-notes *current-track-num*)
                                                        (<ra> :delete-pitchnum Num *current-track-num*)
                                                        #t)
                            _                      :> #f)))))

#||
;; pitch popup menu
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     *current-track-num*
                     (inside-box (<ra> :get-box track-notes *current-track-num*) $x $y)
                     (match (list (find-node $x $y get-pitchnum-box (<ra> :get-num-pitchnums *current-track-num*)))
                            (existing-box Num Box) :> (begin
                                                        (define (delete-pitch)
                                                          (<ra> :undo-notes *current-track-num*)
                                                          (<ra> :delete-pitchnum Num *current-track-num*))

                                                        (popup-menu "Delete pitch" delete-pitch)
                                                        (list "Glide to next pitch"
                                                              :check )
                                                        #t)
                            _                      :> #f)))))
||#


;; highlight current pitch
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (and *current-track-num*
              (inside-box (<ra> :get-box track-notes *current-track-num*) $x $y)
              (match (list (find-node $x $y get-pitchnum-box (<ra> :get-num-pitchnums *current-track-num*)))
                     (existing-box Num Box) :> (begin
                                                 (set-indicator-pitchnum Num *current-track-num*)
                                                 (set-current-pitchnum Num  *current-track-num*)
                                                 #t)
                     _                      :> #f))))




;; pianoroll
;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define *pianonote-move-start* 'move-start)
(define *pianonote-move-all* 'move-all)
(define *pianonote-move-end* 'move-end)

(define-struct pianonote-info
  :tracknum
  :notenum
  :pianonotenum
  :move-type     ;; A "*pianonote-move-<...>*" value
  :mouse-delta
  :note-id
  )


(define (get-pianonote-y pianonotenum notenum tracknum move-type)
  (define y1 (<ra> :get-pianonote-y1 pianonotenum
                                  notenum
                                  tracknum))
  (define y2 (<ra> :get-pianonote-y2 pianonotenum
                                  notenum
                                  tracknum))

  (cond ((eq? move-type
              *pianonote-move-start*)
         y1)
        ((eq? move-type
              *pianonote-move-end*)
         y2)
        (else
         (/ (+ y1 y2) 2))))

         
(define (get-pianonote-box $tracknum $notenum $num)
  ;;(c-display "get-pitchnum-box" $num)
  (make-box2 (<ra> :get-pianonote-x1 $num $notenum $tracknum)
             (<ra> :get-pianonote-y1 $num $notenum $tracknum)
             (+ 2 (<ra> :get-pianonote-x2 $num $notenum $tracknum))
             (+ 2 (<ra> :get-pianonote-y2 $num $notenum $tracknum))))

(define (get-pianonote-move-type $y $y1 $y2)
  (define height (- $y2 $y1))
  (define h (if (< height
                   (* 3 (<ra> :get-half-of-node-width)))
                (/ height
                   3)
                (<ra> :get-half-of-node-width)))                   
  (cond ((< $y (+ $y1 h))
         *pianonote-move-start*)
        ((> $y (- $y2 h))
         *pianonote-move-end*)
        (else
         *pianonote-move-all*)))

(define (get-pianonote-info4 $x $y $tracknum $notenum $pianonotenum)
  (define box (get-pianonote-box $tracknum $notenum $pianonotenum))
  (and (inside-box box $x $y)
       (let ((move-type (get-pianonote-move-type $y (box :y1) (box :y2))))
         (make-pianonote-info :tracknum $tracknum
                              :notenum $notenum
                              :pianonotenum $pianonotenum
                              :move-type move-type
                              :mouse-delta (- $y (get-pianonote-y $pianonotenum $notenum $tracknum move-type))
                              :note-id -1
                              ))))
  
(define-match get-pianonote-info3
  _ _ ________ _______ Num-pianonotes Num-pianonotes :> #f
  X Y Tracknum Notenum Pianonotenum   Num-pianonotes :> (or (get-pianonote-info4 X Y
                                                                                 Tracknum
                                                                                 Notenum
                                                                                 Pianonotenum)
                                                            (get-pianonote-info3 X Y
                                                                                 Tracknum
                                                                                 Notenum
                                                                                 (1+ Pianonotenum)
                                                                                 Num-pianonotes)))

(define-match get-pianonote-info2
  _ _ ________ Num-notes Num-notes :> #f
  X Y Tracknum Notenum   Num-notes :> (or (get-pianonote-info3 X Y
                                                               Tracknum
                                                               Notenum
                                                               0
                                                               (<ra> :get-num-pianonotes Notenum Tracknum))
                                          (get-pianonote-info2 X Y
                                                               Tracknum
                                                               (1+ Notenum)
                                                               Num-notes)))
  
(define (get-pianonote-info $x $y $tracknum)
  (get-pianonote-info2 $x $y $tracknum 0 (<ra> :get-num-notes $tracknum)))


(define (call-get-existing-node-info-callbacks callback info)

  (define num-pianonotes (<ra> :get-num-pianonotes (info :notenum) (info :tracknum)))
  (define pianonotenum (info :pianonotenum))

  (define logtype-holding (= *logtype-hold* (<ra> :get-pianonote-logtype (info :pianonotenum)
                                                                         (info :notenum)
                                                                         (info :tracknum))))


  (define portamento-enabled (<ra> :portamento-enabled (info :notenum)
                                                    (info :tracknum)))
  
  (define is-end-pitch (and portamento-enabled
                            (not logtype-holding)
                            (eq? (info :move-type) *pianonote-move-end*)
                            (= (1- num-pianonotes) pianonotenum)))
  

  (define value-pianonote-num (if (and (not logtype-holding)
                                       (eq? (info :move-type) *pianonote-move-end*))
                                  (1+ (info :pianonotenum))
                                  (info :pianonotenum)))
                                  
  (callback info
            (if is-end-pitch 
                (<ra> :get-note-end-pitch (info :notenum)
                                       (info :tracknum))
                (<ra> :get-pianonote-value (if portamento-enabled
                                            value-pianonote-num
                                            0)
                                        (info :notenum)
                                        (info :tracknum)))
            (if (eq? *pianonote-move-end* (info :move-type))
                (<ra> :get-pianonote-y2 (info :pianonotenum)
                                     (info :notenum)
                                     (info :tracknum))
                (<ra> :get-pianonote-y1 (info :pianonotenum)
                                     (info :notenum)
                                     (info :tracknum)))))
  

(define (create-play-pianonote note-id pianonote-id)
  (let ((instrument-id (<ra> :get-instrument-for-track  *current-track-num*)))
    (if (>= instrument-id 0)
        (<ra> :play-note
              (<ra> :get-pianonote-value pianonote-id note-id *current-track-num*)
              (if (<ra> :get-track-volume-on-off *current-track-num*)
                  (<ra> :get-track-volume *current-track-num*)
                  1.0)
              (if (<ra> :get-track-pan-on-off *current-track-num*)
                  (<ra> :get-track-pan *current-track-num*)
                  0.0)
              (<ra> :get-track-midi-channel *current-track-num*)
              instrument-id)
        -1)))
  
(add-node-mouse-handler :Get-area-box (lambda ()
                                        (and *current-track-num*
                                             (<ra> :pianoroll-visible *current-track-num*)
                                             (<ra> :get-box track-pianoroll *current-track-num*)))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  (and *current-track-num*
                                                       (<ra> :pianoroll-visible *current-track-num*)
                                                       (<ra> :get-box track-pianoroll *current-track-num*)
                                                       (inside-box (<ra> :get-box track-pianoroll *current-track-num*) X Y)
                                                       (let ((info (get-pianonote-info X Y *current-track-num*)))
                                                         ;;(and info
                                                         ;;     (c-display "        NUM " (info :pianonotenum) " type: " (info :move-type)))
                                                         (and info
                                                              (let ((info (copy-pianonote-info info
                                                                                               :note-id (create-play-pianonote (info :notenum)
                                                                                                                               (info :pianonotenum)))))
                                                                (call-get-existing-node-info-callbacks callback info))))))
                        :Get-min-value (lambda (_) 1)
                        :Get-max-value (lambda (_) 127)
                        :Get-x (lambda (info) (/ (+ (<ra> :get-pianonote-x1 (info :pianonotenum)
                                                                         (info :notenum)
                                                                         (info :tracknum))
                                                    (<ra> :get-pianonote-x2 (info :pianonotenum)
                                                                         (info :notenum)
                                                                         (info :tracknum)))
                                                 2))
                        :Get-y (lambda (info)
                                 (+ (info :mouse-delta)
                                    (get-pianonote-y (info :pianonotenum)
                                                     (info :notenum)
                                                     (info :tracknum)
                                                     (info :move-type))))
                        :Make-undo (lambda (_) (<ra> :undo-notes *current-track-num*))
                        :Create-new-node (lambda (X Place callback)
                                           ;;(c-display "Create" X Place)
                                           (define Value (scale X
                                                                (<ra> :get-track-pianoroll-x1 *current-track-num*)
                                                                (<ra> :get-track-pianoroll-x2 *current-track-num*)
                                                                (<ra> :get-pianoroll-low-key *current-track-num*)
                                                                (<ra> :get-pianoroll-high-key *current-track-num*)))
                                           (define Next-Place (get-next-place-from-y *left-button* (<ra> :get-mouse-pointer-y)))
                                           (define Num (<ra> :add-pianonote Value Place Next-Place *current-track-num*))
                                           (if (= -1 Num)
                                               #f
                                               (callback (make-pianonote-info :tracknum *current-track-num*
                                                                              :notenum Num
                                                                              :pianonotenum 0
                                                                              :move-type *pianonote-move-end*
                                                                              :mouse-delta 0
                                                                              :note-id (create-play-pianonote Num 0))
                                                         Value)))
                        :Publicize (lambda (pianonote-info)
                                     (set-current-pianonote (pianonote-info :pianonotenum)
                                                            (pianonote-info :notenum)
                                                            (pianonote-info :tracknum)))
                        :Move-node (lambda (pianonote-info Value Place)
                                     ;;(c-display "moving to. Value: " Value ", Place: " Place " type: " (pianonote-info :move-type) " pianonotenum:" (pianonote-info :pianonotenum))
                                     (define func
                                       (cond ((eq? (pianonote-info :move-type)
                                                   *pianonote-move-start*)
                                              ra:move-pianonote-start)
                                             ((eq? (pianonote-info :move-type)
                                                   *pianonote-move-end*)
                                              ra:move-pianonote-end)
                                             ((eq? (pianonote-info :move-type)
                                                   *pianonote-move-all*)
                                              ra:move-pianonote)
                                             (else
                                              (c-display "UNKNOWN pianonote-info type: " (pianonote-info :move-type))
                                              #f)))
                                     ;(c-display "value:" (<ra> :ctrl-pressed) (if (<ra> :ctrl-pressed)
                                     ;                                             Value
                                     ;                                             (round Value))
                                     ;           Value)
                                     (define new-notenum
                                       (func (pianonote-info :pianonotenum)
                                             (if (<ra> :ctrl-pressed)
                                                 Value
                                                 (round Value))
                                             (or Place -1)
                                             (pianonote-info :notenum)
                                             (pianonote-info :tracknum)))

                                     (if (not (= -1 (pianonote-info :note-id)))
                                         (let ((instrument-id (<ra> :get-instrument-for-track  *current-track-num*)))
                                           (if (>= instrument-id 0)
                                               (<ra> :change-note-pitch
                                                     (<ra> :get-pianonote-value (pianonote-info :pianonotenum) new-notenum *current-track-num*)
                                                     (pianonote-info :note-id)
                                                     (<ra> :get-track-midi-channel *current-track-num*)
                                                     instrument-id))))
                                           
                                     (make-pianonote-info :tracknum (pianonote-info :tracknum)
                                                          :notenum new-notenum
                                                          :pianonotenum (pianonote-info :pianonotenum)
                                                          :move-type (pianonote-info :move-type)
                                                          :mouse-delta (pianonote-info :mouse-delta)
                                                          :note-id (pianonote-info :note-id)
                                                          ))

                        :Release-node (lambda (pianonote-info)
                                        (if (not (= -1 (pianonote-info :note-id)))
                                            (let ((instrument-id (<ra> :get-instrument-for-track  *current-track-num*)))
                                              (if (>= instrument-id 0)
                                                  (<ra> :stop-note (pianonote-info :note-id)
                                                                   (<ra> :get-track-midi-channel *current-track-num*)
                                                                   instrument-id)))))
                                     
                        :Get-pixels-per-value-unit (lambda (_)
                                                     (<ra> :get-pianoroll-low-key *current-track-num*)
                                                     (<ra> :get-pianoroll-high-key *current-track-num*)
                                                     (<ra> :get-half-of-node-width))
                        )


;; highlight current pianonote
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (and *current-track-num*
              (<ra> :pianoroll-visible *current-track-num*)
              (inside-box (<ra> :get-box track-pianoroll *current-track-num*) $x $y)
              ;;(c-display "current-tracknum:" *current-track-num*)
              (let ((pianonote-info (get-pianonote-info $x $y *current-track-num*)))
                '(c-display $x $y pianonote-info
                            (box-to-string (get-pianonote-box 0 1 0)))
                (if (and pianonote-info
                         (let ((pianonote-info pianonote-info)) ;;;(copy-pianonote-info :note-id (create-play-pianonote (pianonote-info :notenum)
                                                                ;;;                                    (pianonote-info :pianonotenum)))))
                           ;;(c-display "type: " (pianonote-info :move-type))
                           (set-current-pianonote (pianonote-info :pianonotenum)
                                                  (pianonote-info :notenum)
                                                  (pianonote-info :tracknum))
                           ;;(c-display "hello:" (pianonote-info :dir))
                           (cond ((eq? (pianonote-info :move-type)
                                       *pianonote-move-start*)
                                  #t)
                                 ((eq? (pianonote-info :move-type)
                                       *pianonote-move-end*)
                                  #t)
                                 ((eq? (pianonote-info :move-type)
                                       *pianonote-move-all*)
                                  #f)))
                         )
                    (set-mouse-pointer ra:set-vertical-resize-mouse-pointer)
                    (set-mouse-pointer ra:set-pointing-mouse-pointer))))))

;; Delete pianonote (shift + right mouse)
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     (<ra> :shift-pressed)
                     *current-track-num*
                     (<ra> :pianoroll-visible *current-track-num*)
                     (inside-box (<ra> :get-box track-pianoroll *current-track-num*) $x $y)
                     (let ((pianonote-info (get-pianonote-info $x $y *current-track-num*)))
                       (and pianonote-info
                            (begin
                              (<ra> :undo-notes (pianonote-info :tracknum))
                              (<ra> :delete-pianonote 0
                                    (pianonote-info :notenum)
                                    (pianonote-info :tracknum))
                              #t)))))))

                               
;; delete note / add pitch / delete pitch
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     *current-track-num*
                     (<ra> :pianoroll-visible *current-track-num*)
                     (inside-box (<ra> :get-box track-pianoroll *current-track-num*) $x $y)
                     (let ((pianonote-info (get-pianonote-info $x $y *current-track-num*)))
                       (if pianonote-info
                           (begin
                             (define (delete-note)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (<ra> :delete-pianonote
                                     0
                                     (pianonote-info :notenum)
                                     (pianonote-info :tracknum))
                               #f)
                             (define (cut-note)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (define Place (get-place-from-y $button $y))
                               (<ra> :cut-note Place
                                               (pianonote-info :notenum)
                                               (pianonote-info :tracknum))
                               #f)
                             (define (delete-pitch)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (<ra> :delete-pianonote (if (= 0 (pianonote-info :pianonotenum))
                                                        1
                                                        (pianonote-info :pianonotenum))
                                                    (pianonote-info :notenum)
                                                    (pianonote-info :tracknum))
                               #f)
                             (define (enable-portamento)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (<ra> :enable-portamento (pianonote-info :notenum)
                                                     (pianonote-info :tracknum))
                               #f)
                             (define (disable-portamento)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (<ra> :disable-portamento (pianonote-info :notenum)
                                                      (pianonote-info :tracknum))
                               #f)
                             (define (set-linear!)
                               (<ra> :set-pianonote-logtype *logtype-linear*
                                                            (pianonote-info :pianonotenum)
                                                            (pianonote-info :notenum)
                                                            (pianonote-info :tracknum))
                               #f
                               )
                             (define (set-hold!)
                               (<ra> :set-pianonote-logtype *logtype-hold*
                                                            (pianonote-info :pianonotenum)
                                                            (pianonote-info :notenum)
                                                            (pianonote-info :tracknum))
                               #f
                               )
                             (define (add-pitch)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (define Place (get-place-from-y $button $y))
                               (define Value (<ra> :get-note-value (pianonote-info :notenum) (pianonote-info :tracknum)))
                               (<ra> :add-pianonote-pitch Value Place (pianonote-info :notenum) (pianonote-info :tracknum))
                               #f)

                             (define (glide-from-here-to-next-note)
                               (<ra> :undo-notes (pianonote-info :tracknum))
                               (define Place (get-place-from-y $button $y))
                               (define Value (<ra> :get-note-value (pianonote-info :notenum) (pianonote-info :tracknum)))
                               (define next-pitch (<ra> :get-note-value (1+ (pianonote-info :notenum)) (pianonote-info :tracknum)))                                 
                               (<ra> :add-pianonote-pitch Value Place (pianonote-info :notenum) (pianonote-info :tracknum))
                               (<ra> :set-note-end-pitch next-pitch (pianonote-info :notenum) (pianonote-info :tracknum))
                               #f)
                             
                             (define (can-glide-from-here-to-next-note?)
                               (and (< (pianonote-info :notenum)
                                       (1- (<ra> :get-num-notes (pianonote-info :tracknum))))
                                    (= (pianonote-info :pianonotenum)
                                       (1- (<ra> :get-num-pianonotes (pianonote-info :notenum)
                                                                     (pianonote-info :tracknum))))))
                               
                             (define num-pianonotes (<ra> :get-num-pianonotes (pianonote-info :notenum)
                                                                           (pianonote-info :tracknum)))
                             (let ((portamento-enabled (<ra> :portamento-enabled
                                                             (pianonote-info :notenum)
                                                             (pianonote-info :tracknum)))
                                   (is-holding (= *logtype-hold* (<ra> :get-pianonote-logtype
                                                                       (pianonote-info :pianonotenum)
                                                                       (pianonote-info :notenum)
                                                                       (pianonote-info :tracknum)))))
                               
                               (popup-menu "Cut Note at mouse position" cut-note
                                           "Glide to next note at mouse position" :enabled (can-glide-from-here-to-next-note?) glide-from-here-to-next-note
                                           "Delete Note" delete-note
                                           "--------"
                                           "Delete break point" :enabled (> num-pianonotes 1) delete-pitch
                                           "Add break point at mouse position" add-pitch
                                           (list "Glide to next break point"
                                                 :check (if (< num-pianonotes 2)
                                                            portamento-enabled
                                                            (not is-holding))
                                                 ;;:enabled (> num-pianonotes 1)
                                                 (lambda (maybe)
                                                   (c-display "   ______________________________   Glide1 called " maybe)
                                                   (if (< num-pianonotes 2)
                                                       (if maybe
                                                           (enable-portamento)
                                                           (disable-portamento))
                                                       (if maybe
                                                           (set-linear!)
                                                           (set-hold!)))))
                                           (let ((note-spans-last-place (note-spans-last-place (pianonote-info :notenum)
                                                                                               (pianonote-info :tracknum))))
                                             (list "continue playing note into next block"
                                                   :check (and note-spans-last-place
                                                               (<ra> :note-continues-next-block (pianonote-info :notenum) (pianonote-info :tracknum)))
                                                   :enabled note-spans-last-place
                                                   (lambda (maybe)
                                                     (<ra> :set-note-continue-next-block maybe (pianonote-info :notenum) (pianonote-info :tracknum)))))


                                           ;;"--------"
                                           ;;"Glide to end position" :check portamento-enabled :enabled (< num-pianonotes 2) (lambda (ison)
                                           ;;                                                                                  (c-display "   ______________________________   Glide2 called " ison)
                                           ;;                                                                                  (if ison
                                           ;;                                                                                      (enable-portamento)
                                           ;;                                                                                      (disable-portamento)))
                                           
                                           ;; "Stop note here" stop-note
                                           ))))
                       #f)))))


;; velocities
;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (get-velocity-box Tracknum Notenum Velocitynum)
  (get-common-node-box (<ra> :get-velocity-x Velocitynum Notenum Tracknum)
                       (<ra> :get-velocity-y Velocitynum Notenum Tracknum)))

(define-struct velocity-info
  :tracknum
  :notenum
  :velocitynum
  :value
  :y  
  )

(define (velocity-info-rating Y Vi)
  (define velocity-y (<ra> :get-velocity-y (Vi :velocitynum) (Vi :notenum) (Vi :tracknum)))
  (cond ((and (= 0
                 (Vi :velocitynum))
              (> Y
                 velocity-y))
         10)
        ((and (= (1- (<ra> :get-num-velocities (Vi :notenum) (Vi :tracknum)))
                 (Vi :velocitynum))
              (< Y
                 velocity-y))
         10)
        (else
         0)))

(define (highest-rated-velocity-info-0 Y A B)
  (if (> (velocity-info-rating Y A)
         (velocity-info-rating Y B))
      A
      B))
         
(define-match highest-rated-velocity-info
  _ (#f       ) :> #f
  Y (#f . Rest) :> (highest-rated-velocity-info Y Rest)
  _ (A        ) :> A
  Y (A  . Rest) :> (let ((B (highest-rated-velocity-info Y Rest)))
                     (if B
                         (highest-rated-velocity-info-0 Y A B)
                         A)))

#||
(highest-rated-velocity-info 'b )
(highest-rated-velocity-info 'b 2)
(highest-rated-velocity-info 'b #f 3)
(highest-rated-velocity-info 'b 4 #f)
||#

(define-match get-velocity-2
  X Y Tracknum Notenum Velocitynum Velocitynum      :> #f
  X Y Tracknum Notenum Velocitynum Total-Velocities :> (begin                                                                     
                                                         (define box (get-velocity-box Tracknum Notenum Velocitynum))
                                                         (if (> (box :y1) Y)
                                                             #f
                                                             (highest-rated-velocity-info Y
                                                                                          (list (get-velocity-2 X Y Tracknum Notenum (1+ Velocitynum) Total-Velocities)
                                                                                                (and (inside-box box X Y)
                                                                                                     (make-velocity-info :velocitynum Velocitynum
                                                                                                                         :notenum Notenum
                                                                                                                         :tracknum Tracknum
                                                                                                                         :value (<ra> :get-velocity-value Velocitynum Notenum Tracknum)
                                                                                                                         :y (box :y)
                                                                                                                         )))))))

(define-match get-velocity-1
  X Y Tracknum Notenum Notenum     :> #f
  X Y Tracknum Notenum Total-Notes :> (highest-rated-velocity-info Y
                                                                   (list (get-velocity-1 X Y Tracknum (1+ Notenum) Total-Notes)
                                                                         (get-velocity-2 X Y Tracknum Notenum 0 (<ra> :get-num-velocities Notenum Tracknum)))))
                                   
  
(define-match get-velocity-0
  X Y -1       :> #f
  X Y Tracknum :> #f :where (>= Tracknum (<ra> :get-num-tracks))
  X Y Tracknum :> (get-velocity-1 X Y Tracknum 0 (<ra> :get-num-notes Tracknum)))
  
(define-match get-velocity-info
  X Y #f       :> (get-velocity-info X Y 0)
  X Y Tracknum :> (highest-rated-velocity-info Y
                                               (list (get-velocity-0 X Y (1- Tracknum)) ;; node in the prev track can overlap into the current track
                                                     (get-velocity-0 X Y Tracknum))))


#||
(let ((node (get-velocity-info 319 169 0)))
  (c-display (node :velocitynum) (node :notenum) (node :tracknum)))
        

(<ra> :get-velocity-x 1 0 0)
(<ra> :get-velocity-y 1 0 0)
||#

(define *current-note-num* #f)


(define-match get-note-num-0
  _____ ________ Num Num   :> #f
  Place Subtrack Num Total :> (if (and (>= Place
                                           (<ra> :get-note-start Num *current-track-num*))
                                       (<  Place
                                           (<ra> :get-note-end Num *current-track-num*))
                                       (=  Subtrack
                                           (<ra> :get-note-subtrack Num *current-track-num*)))
                                  Num
                                  (get-note-num-0 Place
                                                  Subtrack
                                                  (1+ Num)
                                                  Total)))
                                                 
(define-match get-note-num
  X Y :> (get-note-num-0 (<ra> :get-place-from-y Y)
                         *current-subtrack-num*
                         0
                         (<ra> :get-num-notes *current-track-num*)))

;; Set *current-note-num*
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (set! *current-note-num* (and *current-subtrack-num*
                                       (get-note-num $x $y)))))

         
(define-match get-shortest-velocity-distance-0
  ___ _________ X Y X1 Y1 X2 Y2 :> (get-distance X Y X1 Y1 X2 Y2) :where (and (>= Y Y1)
                                                                              (< Y Y2))
  Vel Vel       _ _ __ __ __ __ :> #f
  Vel Total-vel X Y __ __ X2 Y2 :> (get-shortest-velocity-distance-0 (1+ Vel)
                                                                     Total-vel
                                                                     X Y
                                                                     X2 Y2
                                                                     (<ra> :get-velocity-x Vel *current-note-num* *current-track-num*)
                                                                     (<ra> :get-velocity-y Vel *current-note-num* *current-track-num*)))

(define (get-shortest-velocity-distance X Y)
  (if (not *current-note-num*)
      #f
      (get-shortest-velocity-distance-0 2
                                        (<ra> :get-num-velocities *current-note-num* *current-track-num*)
                                        X Y
                                        (<ra> :get-velocity-x 0 *current-note-num* *current-track-num*)
                                        (<ra> :get-velocity-y 0 *current-note-num* *current-track-num*)
                                        (<ra> :get-velocity-x 1 *current-note-num* *current-track-num*)
                                        (<ra> :get-velocity-y 1 *current-note-num* *current-track-num*))))



;; add and move velocity
(add-node-mouse-handler :Get-area-box (lambda ()
                                        (and *current-track-num*
                                             (<ra> :get-box track-fx *current-track-num*)))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  (and *current-track-num*
                                                       (let ((velocity-info (get-velocity-info X Y *current-track-num*)))
                                                         ;;(c-display "vel-info:" velocity-info)
                                                         (and velocity-info
                                                              (callback velocity-info
                                                                        (velocity-info :value)
                                                                        (velocity-info :y))))))
                        :Get-min-value (lambda (_) 0.0)
                        :Get-max-value (lambda (_) 1.0)
                        :Get-x (lambda (info) (<ra> :get-velocity-x (info :velocitynum)
                                                                 (info :notenum)
                                                                 (info :tracknum)))
                        :Get-y (lambda (info) (<ra> :get-velocity-y (info :velocitynum)
                                                                 (info :notenum)
                                                                 (info :tracknum)))
                        :Make-undo (lambda (_) (<ra> :undo-notes *current-track-num*))
                        :Create-new-node (lambda (X Place callback)
                                           ;;(c-display "a" Place)
                                           (and *current-note-num*
                                                (not (get-current-fxnum))
                                                (begin
                                                  ;;(c-display "b" Place)
                                                  (define Value (scale X
                                                                       (<ra> :get-subtrack-x1 *current-subtrack-num* *current-track-num*)
                                                                       (<ra> :get-subtrack-x2 *current-subtrack-num* *current-track-num*)
                                                                       0 1))
                                                  (define Num (<ra> :add-velocity Value Place *current-note-num* *current-track-num*))
                                                  (if (= -1 Num)
                                                      #f
                                                      (callback (make-velocity-info :tracknum *current-track-num*
                                                                                    :notenum *current-note-num*
                                                                                    :velocitynum Num
                                                                                    :value Value
                                                                                    :y #f ;; dont need it.
                                                                                    )
                                                                (<ra> :get-velocity-value Num *current-note-num* *current-track-num*))))))
                        :Publicize (lambda (velocity-info)
                                              (set-indicator-velocity-node (velocity-info :velocitynum)
                                                                           (velocity-info :notenum)
                                                                           (velocity-info :tracknum))
                                              (define value (<ra> :get-velocity-value (velocity-info :velocitynum)
                                                                                   (velocity-info :notenum)
                                                                                   (velocity-info :tracknum)))
                                              (set-velocity-statusbar-text value))
                        :Move-node (lambda (velocity-info Value Place)
                                     (define note-num (<ra> :set-velocity Value (or Place -1) (velocity-info :velocitynum) (velocity-info :notenum) (velocity-info :tracknum)))
                                     (make-velocity-info :tracknum (velocity-info :tracknum)
                                                         :notenum note-num
                                                         :velocitynum (velocity-info :velocitynum)
                                                         :value (velocity-info :value)
                                                         :y (velocity-info :y)))
                        )

;; delete velocity (shift + right mouse)
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     (<ra> :shift-pressed)
                     *current-track-num*
                     (inside-box-forgiving (<ra> :get-box track *current-track-num*) X Y)
                     (begin
                       (define velocity-info (get-velocity-info X Y *current-track-num*))
                       ;;(c-display "got velocity info " velocity-info)
                       (and velocity-info
                            (begin
                              (<ra> :undo-notes (velocity-info :tracknum))
                              (<ra> :delete-velocity
                                    (velocity-info :velocitynum)
                                    (velocity-info :notenum)
                                    (velocity-info :tracknum))
                              #t)))))))

;; velocity popup
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     *current-track-num*
                     (inside-box-forgiving (<ra> :get-box track *current-track-num*) X Y)
                     (begin
                       (define velocity-info (get-velocity-info X Y *current-track-num*))
                       ;;(c-display "got velocity info " velocity-info)
                       (if velocity-info
                           (begin
                             (define (delete-velocity!)
                               (<ra> :undo-notes (velocity-info :tracknum))
                               (<ra> :delete-velocity
                                     (velocity-info :velocitynum)
                                     (velocity-info :notenum)
                                     (velocity-info :tracknum)))
                             (define (set-hold!)
                               (<ra> :undo-notes (velocity-info :tracknum))
                               (<ra> :set-velocity-logtype
                                     *logtype-hold*
                                     (velocity-info :velocitynum)
                                     (velocity-info :notenum)
                                     (velocity-info :tracknum)))
                             (define (set-linear!)
                               (<ra> :undo-notes (velocity-info :tracknum))
                               (<ra> :set-velocity-logtype
                                     *logtype-linear*
                                     (velocity-info :velocitynum)
                                     (velocity-info :notenum)
                                     (velocity-info :tracknum)))
                             (let* ((is-holding (= (<ra> :get-velocity-logtype
                                                         (velocity-info :velocitynum)
                                                         (velocity-info :notenum)
                                                         (velocity-info :tracknum))
                                                   *logtype-hold*))
                                    (num-nodes (<ra> :get-num-velocities (velocity-info :notenum) (velocity-info :tracknum)))
                                    (is-last-node (= (velocity-info :velocitynum)
                                                     (1- num-nodes)))
                                    (note-spans-last-place (note-spans-last-place (velocity-info :notenum)
                                                                                  (velocity-info :tracknum))))
                               '(c-display "place" (<ra> :get-velocity-place
                                                        (velocity-info :velocitynum)
                                                        (velocity-info :notenum)
                                                        (velocity-info :tracknum))
                                          (-line (<ra> :get-num-lines)))
                               (popup-menu "Delete Velocity" delete-velocity!
                                           (list "glide"
                                                 :check (and (not is-holding)
                                                             (not is-last-node))
                                                 :enabled (not is-last-node)
                                                 (lambda (maybe)
                                                   (if maybe
                                                       (set-linear!)
                                                       (set-hold!))))
                                           (list "continue playing note into next block"
                                                 :check (and note-spans-last-place
                                                             (<ra> :note-continues-next-block (velocity-info :notenum) (velocity-info :tracknum)))
                                                 :enabled note-spans-last-place
                                                 (lambda (maybe)
                                                   (<ra> :set-note-continue-next-block maybe (velocity-info :notenum) (velocity-info :tracknum))))))
                             #t)
                           #f))))))


#||
;; show current velocity
(add-mouse-move-handler
 :move (lambda (Button X Y)
         (and *current-track-num*
              (inside-box-forgiving (<ra> :get-box track *current-track-num*) X Y)
              (begin
                (define velocity-info (get-velocity-info X Y *current-track-num*))
                (c-display "got velocity info " velocity-info)
                (if velocity-info
                    (begin
                      (set-mouse-note (velocity-info :notenum) (velocity-info :tracknum))
                      (c-display "setting current to " (velocity-info :velocitynum))
                      (set-indicator-velocity-node (velocity-info :velocitynum)
                                                   (velocity-info :notenum)
                                                   (velocity-info :tracknum))
                      (set-current-velocity-node (velocity-info :velocitynum) (velocity-info :notenum) (velocity-info :tracknum)))
                    (c-display "no current"))))))
||#

#||
(<ra> :get-num-velocities 0 0)

(<ra> :get-velocitynode-y 0 0)
(<ra> :get-velocitynode-y 2 0)
(<ra> :get-velocity-value 7 1)



||#


;; track borders
;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define-struct trackwidth-info
  :tracknum
  :width
  :y)

(define (may-be-a-resize-point-in-track X Y Tracknum)
  (and (>= X (- (<ra> :get-track-x1 Tracknum)
                2))
       (<= X (+ (<ra> :get-track-x1 Tracknum)
                (<ra> :get-half-of-node-width)))
       (>= Y (+ 4 (<ra> :get-track-volume-on-off-y2)))))
                

(define-match get-resize-point-track
  X _ Tracknum :> (and (>= X (- (<ra> :get-track-fx-x2 (1- Tracknum))
                                2))
                       Tracknum) :where (= Tracknum (<ra> :get-num-tracks)) ;; i.e. to the right of the rightmost track
  X _ Tracknum :> #f       :where (> (<ra> :get-track-x1 Tracknum) X)
  X Y Tracknum :> Tracknum :where (may-be-a-resize-point-in-track X Y Tracknum)
  X Y Tracknum :> (get-resize-point-track X Y (1+ Tracknum)))

(define (get-trackwidth-info X Y)
  (and (inside-box (<ra> :get-box editor) X Y)
       (begin
         (define resize-point-track (get-resize-point-track X Y 0))
         (and resize-point-track
              (let ((tracknum (1- resize-point-track)))
                (make-trackwidth-info :tracknum tracknum
                                      :width    (<ra> :get-track-width tracknum)
                                      :y        Y))))))

#||
(add-delta-mouse-handler
 :press (lambda (Button X Y)
          (and (= Button *left-button*)
               (get-trackwidth-info X Y)))

 :move-and-release (lambda (Button DX DY trackwidth-info)
                     (c-display "hepp")
                     (define tracknum (trackwidth-info :tracknum))
                     (define new-width (+ DX
                                          (trackwidth-info :width)))
                     (<ra> :set-track-width new-width tracknum)
                     (make-trackwidth-info :tracknum tracknum
                                             :width    new-width)))

||#

(add-horizontal-handler :Get-handler-data get-trackwidth-info
                        :Get-x1 (lambda (Trackwidth-info)
                                  (<ra> :get-track-fx-x1 (Trackwidth-info :tracknum)))
                        :Get-x2 (lambda (Trackwidth-info)
                                  (+ 10000
                                     (<ra> :get-track-fx-x1 (Trackwidth-info :tracknum))))
                        :Get-min-value (lambda (_)
                                         0)
                        :Get-max-value (lambda (_)
                                         10000)
                        :Get-x (lambda (Trackwidth-info)
                                 (define tracknum (Trackwidth-info :tracknum))
                                 (if (= tracknum (1- (<ra> :get-num-tracks)))
                                     (<ra> :get-track-fx-x2 tracknum)
                                     (<ra> :get-track-x1 (1+ tracknum))))
                        :Get-value (lambda (Trackwidth-info)
                                     (Trackwidth-info :width))
                        :Make-undo (lambda (_)
                                     (<ra> :undo-track-width))
                        :Move (lambda (Trackwidth-info Value)
                                (define tracknum (Trackwidth-info :tracknum))
                                (<ra> :set-track-width Value tracknum))
                        :Publicize (lambda (_)
                                     #f))
                        
                        

;; switch track note area width
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     *current-track-num*
                     (inside-box (<ra> :get-box track-notes *current-track-num*) X Y)
                     (<ra> :change-track-note-area-width *current-track-num*)
                     (<ra> :select-track *current-track-num*)
                     #f))))


;; fxnodes
;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (get-fxnode-box Tracknum FX-num FX-nodenum)
  (get-common-node-box (<ra> :get-fxnode-x FX-nodenum FX-num Tracknum)
                       (<ra> :get-fxnode-y FX-nodenum FX-num Tracknum)))

#||
(box-to-string (get-fxnode-box 0 0 1))
||#

(define-struct fxnode-info
  :tracknum
  :fxnum
  :fxnodenum
  :value
  :y  
  )

(define (fxnode-info-rating Y Fi)
  (define fxnode-y (<ra> :get-fxnode-y (Fi :fxnodenum) (Fi :fxnum) (Fi :tracknum)))
  (cond ((and (= 0
                 (Fi :fxnodenum))
              (> Y
                 fxnode-y))
         10)
        ((and (= (1- (<ra> :get-num-fxnodes (Fi :fxnum) (Fi :tracknum)))
                 (Fi :fxnodenum))
              (< Y
                 fxnode-y))
         10)
        (else
         0)))

(define (highest-rated-fxnode-info-0 Y A B)
  (if (> (fxnode-info-rating Y A)
         (fxnode-info-rating Y B))
      A
      B))
         
(define-match highest-rated-fxnode-info
  _ (#f       ) :> #f
  Y (#f . Rest) :> (highest-rated-fxnode-info Y Rest)
  _ (A        ) :> A
  Y (A  . Rest) :> (let ((B (highest-rated-fxnode-info Y Rest)))
                     (if B
                         (highest-rated-fxnode-info-0 Y A B)
                         A)))

(define-match get-fxnode-2
  X Y Tracknum Fxnum Fxnodenum Fxnodenum      :> #f
  X Y Tracknum Fxnum Fxnodenum Total-Fxnodes :> (begin                                                                     
                                                  (define box (get-fxnode-box Tracknum Fxnum Fxnodenum))
                                                  (if (> (box :y1) Y)
                                                      #f
                                                      (highest-rated-fxnode-info Y
                                                                                 (list (get-fxnode-2 X Y Tracknum Fxnum (1+ Fxnodenum) Total-Fxnodes)
                                                                                       (and (inside-box box X Y)
                                                                                            (make-fxnode-info :fxnodenum Fxnodenum
                                                                                                              :fxnum Fxnum
                                                                                                              :tracknum Tracknum
                                                                                                              :value (<ra> :get-fxnode-value Fxnodenum Fxnum Tracknum)
                                                                                                              :y (box :y)
                                                                                                              )))))))

(define-match get-fxnode-1
  X Y Tracknum Fxnum Fxnum     :> #f
  X Y Tracknum Fxnum Total-Fxs :> (highest-rated-fxnode-info Y
                                                             (list (get-fxnode-1 X Y Tracknum (1+ Fxnum) Total-Fxs)
                                                                   (get-fxnode-2 X Y Tracknum Fxnum 0 (<ra> :get-num-fxnodes Fxnum Tracknum)))))


(define-match get-fxnode-0
  X Y -1       :> #f
  X Y Tracknum :> #f :where (>= Tracknum (<ra> :get-num-tracks))
  X Y Tracknum :> (get-fxnode-1 X Y Tracknum 0 (<ra> :get-num-fxs Tracknum)))

(define-match get-fxnode-info
  X Y #f       :> (get-fxnode-info X Y 0)
  X Y Tracknum :> (highest-rated-fxnode-info Y
                                             (list (get-fxnode-0 X Y (1- Tracknum)) ;; node in the prev track can overlap into the current track
                                                   (get-fxnode-0 X Y Tracknum))))


#||
(<ra> :get-num-fxs 0)
(let ((node (get-fxnode-info 347 211 0)))
  (c-display "hm?" node)
  (if node
      (c-display "node:" (node :fxnodenum) "value:" (node :value))))
        
(<ra> :get-fxnode-x 0 0 0)
(<ra> :get-fxnode-y 0 0 0)
(<ra> :get-fxnode-value 0 0 0)
||#


(define-struct fx/distance
  :fx
  :distance)

(define *current-fx/distance* #f)

(define (get-current-fxnum)
  (and *current-fx/distance*
       (*current-fx/distance* :fx)))
(define (get-current-fx-distance)
  (and *current-fx/distance*
       (*current-fx/distance* :distance)))
  
  
(define (min-fx/distance A B)
  (cond ((not A)
         B)
        ((not B)
         A)
        ((<= (A :distance) (B :distance))
         A)
        (else
         B)))
          
(define-match get-closest-fx-1
  _______ ___________ Fx X Y X1 Y1 X2 Y2 :> (make-fx/distance :fx Fx
                                                              :distance (get-distance X Y X1 Y1 X2 Y2))
                                            :where (and (>= Y Y1)
                                                        (< Y Y2))
  Nodenum Nodenum     __ _ _ __ __ __ __ :> #f
  Nodenum Total-Nodes Fx X Y __ __ X2 Y2 :> (get-closest-fx-1 (1+ Nodenum)
                                                              Total-Nodes
                                                              Fx
                                                              X Y
                                                              X2 Y2
                                                              (<ra> :get-fxnode-x Nodenum Fx *current-track-num*)
                                                              (<ra> :get-fxnode-y Nodenum Fx *current-track-num*)))

(define-match get-closest-fx-0
  Fx Fx        _ _ :> #f
  Fx Total-Fxs X Y :> (min-fx/distance (get-closest-fx-1 2
                                                         (<ra> :get-num-fxnodes Fx *current-track-num*)
                                                         Fx
                                                         X Y
                                                         (<ra> :get-fxnode-x 0 Fx *current-track-num*)
                                                         (<ra> :get-fxnode-y 0 Fx *current-track-num*)
                                                         (<ra> :get-fxnode-x 1 Fx *current-track-num*)
                                                         (<ra> :get-fxnode-y 1 Fx *current-track-num*))
                                       (get-closest-fx-0 (1+ Fx)
                                                         Total-Fxs
                                                         X
                                                         Y)))
                                                                


(define (get-closest-fx X Y)
  (get-closest-fx-0 0 (<ra> :get-num-fxs *current-track-num*) X Y))

#||
(<ra> :get-num-fxs 0)
||#

;; add and move fxnode
(add-node-mouse-handler :Get-area-box (lambda ()
                                        (and *current-track-num*
                                             (<ra> :get-box track-fx *current-track-num*)))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  (and *current-track-num*
                                                       (let ((fxnode-info (get-fxnode-info X Y *current-track-num*)))
                                                         (and fxnode-info
                                                              (callback fxnode-info (fxnode-info :value) (fxnode-info :y))))))
                        :Get-min-value (lambda (fxnode-info)
                                         (<ra> :get-fx-min-value (fxnode-info :fxnum)))
                        :Get-max-value (lambda (fxnode-info)
                                         (<ra> :get-fx-max-value (fxnode-info :fxnum)))
                        :Get-x (lambda (info) (<ra> :get-fxnode-x (info :fxnodenum)
                                                               (info :fxnum)
                                                               (info :tracknum)))
                        :Get-y (lambda (info) (<ra> :get-fxnode-y (info :fxnodenum)
                                                               (info :fxnum)
                                                               (info :tracknum)))
                        :Make-undo (lambda (_) (<ra> :undo-fxs *current-track-num*))
                        :Create-new-node (lambda (X Place callback)
                                           (define Fxnum (get-current-fxnum))
                                           (and Fxnum
                                                (begin
                                                  (define Value (scale X
                                                                       (<ra> :get-track-fx-x1 *current-track-num*) (<ra> :get-track-fx-x2 *current-track-num*)
                                                                       (<ra> :get-fx-min-value Fxnum) (<ra> :get-fx-max-value Fxnum)))
                                                  (define Nodenum (<ra> :add-fxnode Value Place Fxnum *current-track-num*))
                                                  (if (= -1 Nodenum)
                                                      #f
                                                      (callback (make-fxnode-info :tracknum *current-track-num*
                                                                                  :fxnum Fxnum
                                                                                  :fxnodenum Nodenum
                                                                                  :value Value
                                                                                  :y #f ;; dont need it.
                                                                                  )
                                                                (<ra> :get-fxnode-value Nodenum Fxnum *current-track-num*))))))
                        :Publicize (lambda (fxnode-info)
                                     (set-indicator-fxnode (fxnode-info :fxnodenum)
                                                           (fxnode-info :fxnum)
                                                           (fxnode-info :tracknum))
                                     (<ra> :set-statusbar-text (<ra> :get-fx-string (fxnode-info :fxnodenum) (fxnode-info :fxnum) (fxnode-info :tracknum))))

                        :Move-node (lambda (fxnode-info Value Place)                                     
                                     (<ra> :set-fxnode (fxnode-info :fxnodenum) Value (or Place -1) (fxnode-info :fxnum) (fxnode-info :tracknum))
                                     fxnode-info)
                        )

;; Delete fx node (shift + right mouse)
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button X Y)
                (and (= $button *right-button*)
                     (<ra> :shift-pressed)
                     *current-track-num*
                     (inside-box-forgiving (<ra> :get-box track *current-track-num*) X Y)
                     (begin
                       (define fxnode-info (get-fxnode-info X Y *current-track-num*))
                       (and fxnode-info
                            (begin
                              (<ra> :undo-fxs *current-track-num*)
                              (<ra> :delete-fxnode
                                    (fxnode-info :fxnodenum)
                                    (fxnode-info :fxnum)
                                    (fxnode-info :tracknum))
                              #t)))))))

;; fx popup
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     *current-track-num*
                     (inside-box-forgiving (<ra> :get-box track *current-track-num*) X Y)
                     (begin
                       (define fxnode-info (get-fxnode-info X Y *current-track-num*))
                       ;;(c-display "got fx info " fxnode-info)
                       (if fxnode-info
                           (begin
                             (define (delete-node!)
                               (<ra> :undo-fxs *current-track-num*)
                               (<ra> :delete-fxnode
                                     (fxnode-info :fxnodenum)
                                     (fxnode-info :fxnum)
                                     (fxnode-info :tracknum)))
                             (define (set-hold!)
                               (<ra> :undo-fxs *current-track-num*)
                               (<ra> :set-fxnode-logtype
                                     *logtype-hold*
                                     (fxnode-info :fxnodenum)
                                     (fxnode-info :fxnum)
                                     (fxnode-info :tracknum)))
                             (define (set-linear!)
                               (<ra> :undo-fxs *current-track-num*)
                               (<ra> :set-fxnode-logtype
                                     *logtype-linear*
                                     (fxnode-info :fxnodenum)
                                     (fxnode-info :fxnum)
                                     (fxnode-info :tracknum)))
                             (let* ((is-holding (= (<ra> :get-fxnode-logtype
                                                         (fxnode-info :fxnodenum)
                                                         (fxnode-info :fxnum)
                                                         (fxnode-info :tracknum))
                                                  *logtype-hold*))
                                    (num-nodes (<ra> :get-num-fxnodes (fxnode-info :fxnum) (fxnode-info :tracknum)))
                                    (is-last (= (fxnode-info :fxnodenum)
                                                (1- num-nodes))))
                               (popup-menu "Delete Node" delete-node!
                                           (list "glide"
                                                 :check (and (not is-holding) (not is-last))
                                                 :enabled (not is-last)
                                                 (lambda (maybe)
                                                   (if maybe
                                                       (set-linear!)
                                                       (set-hold!)))))
                               )
                             #t)
                           #f))))))

;; add fx
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     *current-track-num*
                     (inside-box (<ra> :get-box track-fx *current-track-num*) X Y)
                     (<ra> :select-track *current-track-num*)
                     (<ra> :request-fx-mouse-pos)
                     #t))))


(define (get-full-fx-name fxnum tracknum)
  (define fxname (<ra> :get-fx-name fxnum tracknum))
  (define trackinstrument_id (<ra> :get-instrument-for-track tracknum))
  (define fxinstrument_id (<ra> :get-fx-instrument fxnum tracknum))
  (if (= trackinstrument_id fxinstrument_id)
      fxname
      (<-> fxname " (" (<ra> :get-instrument-name fxinstrument_id) ")")))

;; Show and set:
;;  1. current fx or current note, depending on which nodeline is closest to the mouse pointer
;;  2. current velocity node, or current fxnode
;;
(add-mouse-move-handler
 :move (lambda (Button X Y)
         (define resize-mouse-pointer-is-set #f)
         (define result
           (and *current-track-num*
                (inside-box-forgiving (<ra> :get-box track *current-track-num*) X Y)
                (lazy
                  (define-lazy velocity-info (get-velocity-info X Y *current-track-num*))
                  (define-lazy fxnode-info (get-fxnode-info X Y *current-track-num*))
                  
                  (define-lazy velocity-dist (get-shortest-velocity-distance X Y))
                  (define-lazy fx-dist (get-closest-fx X Y))
                  
                  (define-lazy is-in-fx-area (inside-box (<ra> :get-box track-fx *current-track-num*) X Y))
                  
                  (define-lazy velocity-dist-is-shortest
                    (cond ((not velocity-dist)
                           #f)
                          ((not fx-dist)
                           #t)
                          (else
                           ;;(c-display "dist:" fx-dist) ;; :distance))
                           (<= velocity-dist
                               (fx-dist :distance)))))
                  
                  (define-lazy fx-dist-is-shortest
                    (cond ((not fx-dist)
                           #f)
                          ((not velocity-dist)
                           #t)
                          (else
                           (<= (fx-dist :distance)
                               velocity-dist))))
                  
                  
                  (define-lazy trackwidth-info (get-trackwidth-info X Y))
                  (set! *current-fx/distance* #f)
                  
                  (cond (velocity-info
                         (set-mouse-note (velocity-info :notenum) (velocity-info :tracknum))
                         ;;(c-display "setting current to " (velocity-info :velocitynum) (velocity-info :dir))
                         (set-indicator-velocity-node (velocity-info :velocitynum)
                                                      (velocity-info :notenum)
                                                      (velocity-info :tracknum))
                         (set-current-velocity-node (velocity-info :velocitynum) (velocity-info :notenum) (velocity-info :tracknum)))
                        
                        (fxnode-info
                         (set-mouse-fx (fxnode-info :fxnum) (fxnode-info :tracknum))
                         (set-indicator-fxnode (fxnode-info :fxnodenum)
                                               (fxnode-info :fxnum)
                                               (fxnode-info :tracknum))
                         (set-current-fxnode  (fxnode-info :fxnodenum)
                                              (fxnode-info :fxnum)
                                              (fxnode-info :tracknum))
                         )
                        
                        (trackwidth-info
                         (set! resize-mouse-pointer-is-set #t)
                         (set-mouse-pointer ra:set-horizontal-split-mouse-pointer))
                        
                        ((and is-in-fx-area velocity-dist-is-shortest)
                         (set-mouse-note *current-note-num* *current-track-num*))
                        
                        ((and is-in-fx-area fx-dist-is-shortest)
                         (set! *current-fx/distance* fx-dist)
                         (<ra> :set-statusbar-text (get-full-fx-name (fx-dist :fx) *current-track-num*)) ;; TODO: Also write fx value at mouse position.
                         (set-mouse-fx (fx-dist :fx) *current-track-num*)
                         )
                      
                        (else
                         #f)))))
         (if (or (not result)
                 (not resize-mouse-pointer-is-set))
             (if (and (> Y (<ra> :get-block-header-y2))
                      (< Y (<ra> :get-reltempo-slider-y1))
                      *current-track-num*
                      (or (not (<ra> :pianoroll-visible *current-track-num*))
                          (not (inside-box (<ra> :get-box track-pianoroll *current-track-num*) X Y))))
                 (<ra> :set-normal-mouse-pointer)))
         result))



;; move tracker cursor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and ;(= Button *middle-button*)
                 (inside-box (<ra> :get-box editor) X Y)
                 *current-track-num*
                 (<ra> :select-track *current-track-num*)
                 #t))))


;; show/hide time tracks
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     *current-track-num-all-tracks*
                     (>= Y (<ra> :get-block-header-y2))
                     (< Y (<ra> :get-reltempo-slider-y1))
                     (cond ((= *current-track-num-all-tracks* (<ra> :get-rel-tempo-track-num))
                            (c-display "reltempo")
                            (popup-menu "hide tempo multiplier track" ra:show-hide-reltempo-track))
                           ((= *current-track-num-all-tracks* (<ra> :get-tempo-track-num))
                            (c-display "tempo")
                            (popup-menu "hide BPM track" ra:show-hide-bpm-track))
                           ((= *current-track-num-all-tracks* (<ra> :get-lpb-track-num))
                            (c-display "lpb")
                            (popup-menu "hide LPB track" ra:show-hide-lpb-track))
                           ((= *current-track-num-all-tracks* (<ra> :get-signature-track-num))
                            (c-display "signature")
                            (popup-menu "hide time signature track" ra:show-hide-signature-track)))
                     #t))))



;; seqtrack / seqblock
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (for-each-seqblock func)
  (let loop ((seqblocknum 0)
             (seqtracknum 0))
    (cond ((= seqtracknum (<ra> :get-num-seqtracks))
           #t)
          ((= seqblocknum (<ra> :get-num-seqblocks seqtracknum))
           (loop 0 (1+ seqtracknum)))
          (else
           (define ret (func seqtracknum seqblocknum))
           (if (and (pair? ret) (pair? (cdr ret)) (eq? 'stop (car ret)) (null? (cddr ret)))
               (cadr ret)
               (loop (1+ seqblocknum) seqtracknum))))))

(define (for-each-selected-seqblock func)
  (for-each-seqblock (lambda (seqtracknum seqblocknum)
                       (when (<ra> :is-seqblock-selected seqblocknum seqtracknum)
                         ;;(c-display "funcing" seqtracknum seqblocknum)
                         (func seqtracknum seqblocknum)))))
  


;; sequencer tempo automation
;;
(define (get-seqtemponode-box $num)
  (get-common-node-box (<ra> :get-seqtemponode-x $num)
                       (<ra> :get-seqtemponode-y $num)))

(define (get-seqtempo-value Y)
  (define max-tempo (<ra> :get-seqtempo-max-tempo))
  (define y1 (<ra> :get-seqtempo-area-y1))
  (define y2 (<ra> :get-seqtempo-area-y2))
  (define mid (/ (+ y1 y2) 2))
  ;;(c-display Y y1 y2 max-tempo (<= Y mid))
  (if (<= Y mid)
      (scale Y y1 mid max-tempo 1)
      (scale Y mid y2 1 (/ 1 max-tempo))))

(define (get-highest-seqtemp-value)
  (apply max (map (lambda (n)
                    (<ra> :get-seqtempo-value n))
                  (iota (<ra> :get-num-seqtemponodes)))))

(add-node-mouse-handler :Get-area-box (lambda ()
                                        (and (<ra> :seqtempo-visible)
                                             (<ra> :get-box seqtempo-area)))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  (and (<ra> :seqtempo-visible)
                                                       (inside-box-forgiving (<ra> :get-box seqtempo-area) X Y)
                                                       (match (list (find-node-horizontal X Y get-seqtemponode-box (<ra> :get-num-seqtemponodes)))
                                                              (existing-box Num Box) :> (begin
                                                                                          ;;(c-display "EXISTING " Num)
                                                                                          (define Time (scale X
                                                                                                              (<ra> :get-seqtempo-area-x1) (<ra> :get-seqtempo-area-x2)
                                                                                                              (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
                                                                                          (set-grid-type #t)
                                                                                          (callback Num Time Y))
                                                              _                      :> #f)))
                        :Get-min-value (lambda (_)
                                         (<ra> :get-sequencer-visible-start-time))
                        :Get-max-value (lambda (_)
                                         (<ra> :get-sequencer-visible-end-time))
                        :Get-x (lambda (Num) (<ra> :get-seqtemponode-x Num))
                        :Get-y (lambda (Num) (<ra> :get-seqtemponode-y Num))
                        :Make-undo (lambda (_)              
                                     (<ra> :undo-seqtempo))
                        :Create-new-node (lambda (X Y callback)

                                           (define Time (scale X
                                                               (<ra> :get-seqtempo-area-x1) (<ra> :get-seqtempo-area-x2)
                                                               (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
                                           (define PositionTime (if (<ra> :ctrl-pressed)
                                                                    Time
                                                                    (<ra> :find-closest-seqtrack-beat-start 0 (floor Time))))
                                           (define TempoMul (get-seqtempo-value Y))
                                           (define Num (<ra> :add-seqtemponode PositionTime TempoMul *logtype-linear*))
                                           (if (= -1 Num)
                                               #f
                                               (begin
                                                 (set-grid-type #t)
                                                 (callback Num Time))))
                        :Release-node (lambda (Num)
                                        (set-grid-type #f))
                        :Move-node (lambda (Num Time Y)
                                     (define TempoMul (get-seqtempo-value Y))
                                     (define logtype (<ra> :get-seqtempo-logtype Num))
                                     (if (not (<ra> :ctrl-pressed))
                                         (set! Time (<ra> :find-closest-seqtrack-beat-start 0 (floor Time))))
                                     (<ra> :set-seqtemponode Time TempoMul logtype Num)
                                     ;;(c-display "NUM:" Num ", Time:" Time ", TempoMul:" TempoMul)
                                     Num
                                     )
                        :Publicize (lambda (Num) ;; this version works though. They are, or at least, should be, 100% functionally similar.
                                     (<ra> :set-statusbar-text (<-> "Tempo: " (two-decimal-string (<ra> :get-seqtempo-value Num))))
                                     (<ra> :set-curr-seqtemponode Num)
                                     #f)
                        
                        :Use-Place #f
                        :Mouse-pointer-func ra:set-normal-mouse-pointer
                        :Get-pixels-per-value-unit #f
                        )                        


;; delete seqtemponode / popupmenu
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     (<ra> :seqtempo-visible)                     
                     (inside-box-forgiving (<ra> :get-box seqtempo-area) $x $y)
                     (begin
                       (define Num (match (list (find-node-horizontal $x $y get-seqtemponode-box (<ra> :get-num-seqtemponodes)))
                                          (existing-box Num Box) :> Num
                                          _                      :> #f))
                       (if (<ra> :shift-pressed)
                           (begin
                             ;;(c-display "  Deleting" Num)
                             (<ra> :undo-seqtempo)
                             (<ra> :delete-seqtemponode Num))
                           (popup-menu (list "Delete"
                                             :enabled (and Num (> Num 0) (< Num (1- (<ra> :get-num-seqtemponodes))))
                                             (lambda ()
                                               (<ra> :undo-seqtempo)
                                               (<ra> :delete-seqtemponode Num)))
                                       (list "Reset (set value to 1.0)"
                                             :enabled Num
                                             (lambda ()
                                               (<ra> :undo-seqtempo)
                                               (<ra> :set-seqtemponode
                                                     (<ra> :get-seqtempo-abstime Num)
                                                     1.0
                                                     (<ra> :get-seqtempo-logtype Num)
                                                     Num)))
                                       (list "Glide to next break point"
                                             :check (and Num (= (<ra> :get-seqtempo-logtype Num)
                                                                *logtype-linear*))
                                             :enabled Num
                                             (lambda (maybe)
                                               (<ra> :undo-seqtempo)
                                               (<ra> :set-seqtemponode
                                                     (<ra> :get-seqtempo-abstime Num)
                                                     (<ra> :get-seqtempo-value Num)
                                                     (if maybe *logtype-linear* *logtype-hold*)
                                                     Num)))
                                       (list "Set maximum tempo"
                                             (lambda ()
                                               (define highest (get-highest-seqtemp-value))
                                               (define now (<ra> :get-seqtempo-max-tempo))
                                               (define new (<ra> :request-float (<-> "Max tempo automation value (now: "
                                                                                     (two-decimal-string now) " ("
                                                                                     (two-decimal-string highest) "-1000000): ")
                                                                                highest
                                                                                1000000))
                                               (if (> new highest)
                                                   (<ra> :set-seqtempo-max-tempo new))))
                                       (list "Hide song tempo automation"
                                             (lambda ()
                                               (<ra> :set-seqtempo-visible #f)))
                                       ))
                       #t)))))

 
;; highlight current seqtemponode
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (and (<ra> :seqtempo-visible)
              ;;(or (c-display "---" $x $y (box-to-string (<ra> :get-box seqtempo-area)) (inside-box-forgiving (<ra> :get-box seqtempo-area) $x $y)) #t)
              (inside-box-forgiving (<ra> :get-box seqtempo-area) $x $y)
              (match (list (find-node-horizontal $x $y get-seqtemponode-box (<ra> :get-num-seqtemponodes)))
                     (existing-box Num Box) :> (begin
                                                 ;;(c-display "hepp" Num)
                                                 (<ra> :set-statusbar-text (<-> "Tempo: " (two-decimal-string (<ra> :get-seqtempo-value Num))))
                                                 (<ra> :set-curr-seqtemponode Num)
                                                 #t)
                     A                      :> (begin
                                                 ;;(c-display "**Didnt get it:" A)
                                                 (<ra> :set-curr-seqtemponode -1)
                                                 #f)))))



;; sequencer looping
;;


(define (get-seqloop-start-x)
  (scale (<ra> :get-seqlooping-start)
         (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)
         (<ra> :get-seqtimeline-area-x1) (<ra> :get-seqtimeline-area-x2)))

(define (get-seqloop-end-x)
  (scale (<ra> :get-seqlooping-end)
         (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)
         (<ra> :get-seqtimeline-area-x1) (<ra> :get-seqtimeline-area-x2)))


(define (create-seqloop-handler Type)
  (add-horizontal-handler :Get-handler-data (lambda (X Y)
                                              (and (inside-box (<ra> :get-box seqtimeline-area) X Y)
                                                   (let* ((start-x (get-seqloop-start-x))
                                                          (end-x (get-seqloop-end-x))
                                                          (mid (average start-x end-x)))
                                                     (set-grid-type #t)
                                                     (if (eq? Type 'start)
                                                         (and (< X mid)
                                                              (<ra> :get-seqlooping-start))
                                                         (and (> X mid)
                                                              (<ra> :get-seqlooping-end))))))
                          :Get-x1 (lambda (_)
                                    (<ra> :get-seqtimeline-area-x1))
                          :Get-x2 (lambda (_)
                                    (<ra> :get-seqtimeline-area-x2))
                          :Get-min-value (lambda (_)
                                           (<ra> :get-sequencer-visible-start-time))
                          :Get-max-value (lambda (_)
                                           (<ra> :get-sequencer-visible-end-time))
                          :Get-value (lambda (Value)
                                       Value)
                          :Release (lambda ()
                                     (set-grid-type #f))
                          :Make-undo (lambda (_)
                                       50)
                          :Move (lambda (_ Value)
                                  (set! Value (floor Value))
                                  (if (not (<ra> :ctrl-pressed))
                                      (set! Value (<ra> :find-closest-seqtrack-beat-start 0 Value)))
                                  (if (eq? Type 'start)
                                      (<ra> :set-seqlooping-start Value)
                                      (<ra> :set-seqlooping-end Value)))
                          
                          :Publicize (lambda (Value)
                                       (set-statusbar-loop-info Type))
                          
                          :Mouse-pointer-func ra:set-normal-mouse-pointer
                          ))

(create-seqloop-handler 'start)
(create-seqloop-handler 'end)

(define (set-statusbar-loop-info Type)
  (if (eq? Type 'start)
      (<ra> :set-statusbar-text (<-> "Loop start: " (two-decimal-string (/ (<ra> :get-seqlooping-start) (<ra> :get-sample-rate)))))
      (<ra> :set-statusbar-text (<-> "Loop end: " (two-decimal-string (/ (<ra> :get-seqlooping-end) (<ra> :get-sample-rate)))))))

;; highlight loop start / loop end
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (and (inside-box (<ra> :get-box seqtimeline-area) $x $y)
              (let* ((start-x (get-seqloop-start-x))
                     (end-x (get-seqloop-end-x))
                     (mid (average start-x end-x)))
                (set-statusbar-loop-info (if (< $x mid) 'start 'end))
                #t))))

(add-mouse-cycle (make-mouse-cycle
                  :press-func (lambda (Button X Y)                                
                                (if (and (= Button *right-button*)
                                         (inside-box (<ra> :get-box seqtimeline-area) X Y))
                                    (begin
                                      (popup-menu "Play loop"
                                                  :check (<ra> :is-seqlooping)
                                                  (lambda (val)
                                                    (<ra> :set-seqlooping val)))
                                      #t)
                                    #f))))


#||
(/ (<ra> :get-seqlooping-start) (<ra> :get-sample-rate))
(/ (<ra> :get-seqlooping-end) (<ra> :get-sample-rate))
(<ra> :set-seqlooping-end 500000)
||#


#||
(define (find-closest-bar seqtracknum pos)
  (let loop ((seqblocknum 0)
             (last-bar-pos 0)
             (last-bar-len 0)
             (min-dist #f))
    (let* ((after-end (>= seqblocknum (<ra> :get-num-seqblocks seqtracknum)))
           (start (if after-end
                      (+ last-bar-pos last-bar-len)
                      (<ra> :get-seqblock-start-time seqblocknum seqtracknum)))
           (end   (if after-end
                      (+ last-bar-pos (* last-bar-len 2))
                      (<ra> :get-seqblock-end-time seqblocknum seqtracknum))))
      (cond ((and (>= pos start)
                  (< pos end))
             )))))
||#

(define *current-seqtrack-num* #f)

(define (get-seqtracknum X Y)
  (define num-seqtracks (<ra> :get-num-seqtracks))
  (let loop ((seqtracknum 0))
    (cond ((= seqtracknum num-seqtracks)
           #f)
          ((inside-box (ra:get-box2 seqtrack seqtracknum) X Y)
           seqtracknum)
          (else
           (loop (1+ seqtracknum))))))

(add-mouse-move-handler
 :move (lambda (Button X Y)
         (let ((old *current-seqtrack-num*)
               (new (get-seqtracknum X Y)))
           (cond ((and old (not new))
                  (set! *current-seqtrack-num* new))
                 ((and new (not old))
                  ;;(c-display "set-normal")
                  ;;(<ra> :set-normal-mouse-pointer)
                  (set! *current-seqtrack-num* new))
                 (else
                  #f)))))


             
(define-struct seqblock-info
  :seqtracknum
  :seqblocknum)

(define (get-selected-seqblock-infos)
  (define ret '())
  (for-each-selected-seqblock (lambda (seqtracknum seqblocknum)
                                (push-back! ret (make-seqblock-info :seqtracknum seqtracknum
                                                                    :seqblocknum seqblocknum))))
  ret)

(define (get-seqblock seqtracknum X Y)
  (define num-seqblocks (<ra> :get-num-seqblocks seqtracknum))
  (let loop ((seqblocknum 0))
    (cond ((= seqblocknum num-seqblocks)
           #f)
          ((inside-box (ra:get-box2 seqblock seqblocknum seqtracknum) X Y)
           (make-seqblock-info :seqtracknum seqtracknum
                               :seqblocknum seqblocknum))
          (else
           ;;(c-display X Y (box-to-string (ra:get-box2 seqblock seqblocknum seqtracknum)))
           (loop (1+ seqblocknum))))))

(define (get-seqblock-info X Y)
  (and (inside-box (<ra> :get-box sequencer) X Y)
       (begin
         (define seqtracknum *current-seqtrack-num*)
         ;;(c-display "seqtracknum:" seqtracknum X Y (inside-box (ra:get-box2 seqtrack 1) X Y))
         (and seqtracknum
              (get-seqblock seqtracknum X Y)))))

(define (num-seqblocks-in-sequencer)
  (apply +
         (map (lambda (seqtracknum)
                (<ra> :get-num-seqblocks seqtracknum))
              (iota (<ra> :get-num-seqtracks)))))

(define (set-grid-type doit)
  (if doit
      (<ra> :set-sequencer-grid-type 1)
      (<ra> :set-sequencer-grid-type 0)))
      
(define (only-select-one-seqblock dasseqblocknum dasseqtracknum)
  (for-each-seqblock (lambda (seqtracknum seqblocknum)
                       (define should-be-selected (and (= seqtracknum dasseqtracknum)
                                                       (= seqblocknum dasseqblocknum)))
                       (<ra> :select-seqblock should-be-selected seqblocknum seqtracknum))))

(define (delete-all-gfx-gfx-seqblocks)
  (for-each (lambda (seqtracknum)
              (while (> (<ra> :get-num-gfx-gfx-seqblocks seqtracknum) 0)
                (<ra> :delete-gfx-gfx-seqblock 0 seqtracknum)))
            (iota (<ra> :get-num-seqtracks))))

(define gakkgakk-last-inc-time 0)
(define gakkgakk-really-last-inc-time 0)
(define gakkgakk-last-inc-track 0)

(define (get-data-for-seqblock-moving seqblock-infos inc-time inc-track)
  (define num-seqtracks (<ra> :get-num-seqtracks))

  (define skew #f) ;; We want the same skew for all blocks. Use skew for the first block, i.e. the uppermost leftmost one.
  
  (map (lambda (seqblock-info)
         (define seqtracknum (seqblock-info :seqtracknum))
         (define seqblocknum (seqblock-info :seqblocknum))
         (define blocknum (<ra> :get-seqblock-blocknum seqblocknum seqtracknum))
         (define start-time (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
         (define new-seqtracknum (+ seqtracknum inc-track))
         (cond ((and (>= new-seqtracknum 0)
                     (< new-seqtracknum num-seqtracks))
                (define new-pos (floor (+ inc-time start-time)))
                
                (when (not skew)
                  (define new-pos2 (if (or (= 1 (num-seqblocks-in-sequencer))
                                           (<ra> :ctrl-pressed))
                                       new-pos
                                       (<ra> :find-closest-seqtrack-bar-start new-seqtracknum new-pos)))
                  (if (< new-pos2 0)
                      (set! new-pos2 0))
                  (set! skew (- new-pos2 new-pos))
                  (set! gakkgakk-really-last-inc-time (- new-pos2 start-time)))

                ;;(c-display "start-time/inc-time:" start-time inc-time)
                (list new-seqtracknum
                      blocknum
                      (+ skew new-pos)))
               (else
                #f)))
       seqblock-infos))
  
(define (create-gfx-gfx-seqblocks seqblock-infos inc-time inc-track)
  (if (< (+ gakkgakk-smallest-time inc-time) 0)
      (set! inc-time (- gakkgakk-smallest-time)))

  (set! gakkgakk-last-inc-time inc-time)
  (set! gakkgakk-last-inc-track inc-track)
  (for-each (lambda (data)
              (if data
                  (apply ra:add-gfx-gfx-block-to-seqtrack data)))
            (get-data-for-seqblock-moving seqblock-infos inc-time inc-track)))

(define (apply-gfx-gfx-seqblocks seqblock-infos)
  (define inc-time gakkgakk-last-inc-time)
  (define inc-track gakkgakk-last-inc-track)
  
  (define data (get-data-for-seqblock-moving seqblock-infos inc-time inc-track)) ;; Must do this before deleting the old seqblocks.

  (undo-block
   (lambda ()
     (delete-all-selected-seqblocks) ;; Hack. This will only work if seqblock-infos contains the current set of selected blocks. (deleting from seqblock-infos is a little bit tricky since the seqblock indexes changes)
     (for-each (lambda (data)
                 (if data
                  (apply ra:add-block-to-seqtrack data)))
               data))))


;; seqblock move
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define gakkgakk-last-value 0) ;; TODO: Fix.
(define gakkgakk-has-made-undo #f)
(define gakkgakk-start-pos 0)
(define gakkgakk-was-selected #f)

;; Move single seqblock
(add-node-mouse-handler :Get-area-box (lambda()
                                        (<ra> :get-box sequencer))
                        :Get-existing-node-info (lambda (X Y callback)
                                                  (let ((seqtracknum *current-seqtrack-num*))
                                                    (and (not *current-seqautomation/distance*)
                                                         seqtracknum
                                                         (begin
                                                           (<ra> :select-seqtrack seqtracknum)
                                                           (let ((seqblock-info (get-seqblock-info X Y)))
                                                             ;;(c-display "get-existing " seqblock-info X Y)
                                                             (and seqblock-info
                                                                  (let* ((seqtracknum (and seqblock-info (seqblock-info :seqtracknum)))
                                                                         (seqblocknum (and seqblock-info (seqblock-info :seqblocknum)))
                                                                         (is-selected (<ra> :is-seqblock-selected seqblocknum seqtracknum)))

                                                                    (cond ((<= (<ra> :get-num-selected-seqblocks) 1)

                                                                           (if (not (<ra> :is-playing-song))
                                                                               (<ra> :select-block (<ra> :get-seqblock-blocknum seqblocknum seqtracknum)))
                                                                           (cond ((and (not is-selected)
                                                                                       (not (<ra> :ctrl-pressed)))
                                                                                  (only-select-one-seqblock seqblocknum seqtracknum))
                                                                                 (else
                                                                                  (<ra> :select-seqblock #t seqblocknum seqtracknum)))

                                                                           (set-grid-type #t)
                                                                           
                                                                           (set! gakkgakk-has-made-undo #f)                                                                    
                                                                           (set! gakkgakk-start-pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                                                           (set! gakkgakk-was-selected is-selected)
                                                                           
                                                                           (callback seqblock-info gakkgakk-start-pos
                                                                                     Y))
                                                                          (else
                                                                           #f)))))))))
                        :Get-min-value (lambda (seqblock-info)
                                         (define seqtracknum (seqblock-info :seqtracknum))
                                         (define seqblocknum (seqblock-info :seqblocknum))
                                         (if (or #t (= 0 seqblocknum))
                                             0
                                             (<ra> :get-seqblock-end-time (1- seqblocknum) seqtracknum)))
                        :Get-max-value (lambda (seqblock-info)
                                         (define seqtracknum (seqblock-info :seqtracknum))
                                         (define seqblocknum (seqblock-info :seqblocknum))
                                         (define num-seqblocks (<ra> :get-num-seqblocks (seqblock-info :seqtracknum)))
                                         (if (or #t (= (1- num-seqblocks) seqblocknum))
                                             (+ 10000000000 (<ra> :get-seqblock-end-time (seqblock-info :seqblocknum) seqtracknum))
                                             (<ra> :get-seqblock-start-time (1+ seqblocknum) seqtracknum)))
                        :Get-x (lambda (info) #f) ;;(/ (+ (<ra> :get-seqblock-x1 (info :seqblocknum)
                                                  ;;        (info :seqtracknum))
                                                  ;;  (<ra> :get-seqblock-x2 (info :seqblocknum)
                                                  ;;        (info :seqtracknum)))
                                                  ;;2))
                        :Get-y (lambda (info) #f) ;;(/ (+ (<ra> :get-seqblock-y1 (info :seqblocknum)
                                                  ;;        (info :seqtracknum))
                                                  ;;  (<ra> :get-seqblock-y2 (info :seqblocknum)
                                                  ;;        (info :seqtracknum)))
                                                  ;;2))
                        :Make-undo (lambda (_)
                                     #f)
                        :Create-new-node (lambda (X seqtracknum callback)
                                           #f)
                        :Publicize (lambda (seqblock-info)
                                     (if (number? gakkgakk-last-value)
                                         (<ra> :set-statusbar-text (two-decimal-string (/ gakkgakk-last-value
                                                                                          (<ra> :get-sample-rate))))))

                        :Release-node (lambda (seqblock-info)
                                        (define has-moved (not (= gakkgakk-start-pos gakkgakk-last-value)))
                                        (define seqtracknum (seqblock-info :seqtracknum))
                                        (define seqblocknum (seqblock-info :seqblocknum))

                                        (define old-pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                        (define new-pos gakkgakk-last-value)

                                        (if (not (= old-pos new-pos))
                                            (begin
                                              (when (not gakkgakk-has-made-undo)
                                                (<ra> :undo-sequencer)
                                                (set! gakkgakk-has-made-undo #f))
                                              (<ra> :move-seqblock seqblocknum new-pos seqtracknum))
                                            (<ra> :move-seqblock-gfx seqblocknum old-pos seqtracknum))
                                        (set-grid-type #f)
                                        (if (and (<ra> :ctrl-pressed)
                                                 (not has-moved))
                                            (<ra> :select-seqblock (not gakkgakk-was-selected) seqblocknum seqtracknum)
                                            (<ra> :select-seqblock #f seqblocknum seqtracknum))

                                        seqblock-info)

                        :Move-node (lambda (seqblock-info Value Y)
                                     (define seqtracknum (seqblock-info :seqtracknum))
                                     (define seqblocknum (seqblock-info :seqblocknum))
                                     ;;(c-display "  MOVING GFX " (/ new-pos 44100.0))
                                     (define new-seqtracknum (or (get-seqtracknum (1+ (<ra> :get-seqtrack-x1 0)) Y) seqtracknum))
                                     ;;(c-display "  Y" Y new-seqtracknum)

                                     (define new-pos (if (or (= 1 (num-seqblocks-in-sequencer))
                                                             (<ra> :ctrl-pressed))
                                                         (floor Value)
                                                         (<ra> :find-closest-seqtrack-bar-start new-seqtracknum (floor Value))))
                                     (set! gakkgakk-last-value new-pos)
                                     
                                     (set-grid-type #t)

                                     (define (replace-seqblock new-pos dosomething)
                                       (let ((blocknum (<ra> :get-seqblock-blocknum seqblocknum seqtracknum)))
                                         (when (not gakkgakk-has-made-undo)
                                           (<ra> :undo-sequencer)
                                           (set! gakkgakk-has-made-undo #t))
                                         (ignore-undo-block (lambda ()
                                                              ;;(c-display "bef:" (/ (<ra> :get-seqblock-start-time (1+ seqblocknum) seqtracknum) 44100.0))
                                                              (<ra> :delete-seqblock seqblocknum seqtracknum)
                                                              ;;(c-display "aft:" (/ (<ra> :get-seqblock-start-time seqblocknum seqtracknum) 44100.0))
                                                              (if dosomething
                                                                  (dosomething))
                                                              (define new-seqblocknum (<ra> :add-block-to-seqtrack new-seqtracknum blocknum new-pos))
                                                              (<ra> :select-seqblock #t new-seqblocknum new-seqtracknum)
                                                              (make-seqblock-info :seqtracknum new-seqtracknum
                                                                                  :seqblocknum new-seqblocknum)))))                     

                                     (define (swap-blocks seqblocknum ret-seqblocknum new-pos1 new-pos2)
                                       (when (not gakkgakk-has-made-undo)
                                         (<ra> :undo-sequencer)
                                         (set! gakkgakk-has-made-undo #t))

                                       (define blocknum1 (<ra> :get-seqblock-blocknum seqblocknum seqtracknum))
                                       (define blocknum2 (<ra> :get-seqblock-blocknum (1+ seqblocknum) seqtracknum))

                                       (ignore-undo-block (lambda ()
                                                            (<ra> :delete-seqblock seqblocknum seqtracknum)
                                                            (<ra> :delete-seqblock seqblocknum seqtracknum)
                                                            (<ra> :add-block-to-seqtrack seqtracknum blocknum2 new-pos2)
                                                            (<ra> :add-block-to-seqtrack seqtracknum blocknum1 new-pos1)
                                                            (<ra> :select-seqblock #t ret-seqblocknum new-seqtracknum)
                                                            (make-seqblock-info :seqtracknum new-seqtracknum
                                                                                :seqblocknum ret-seqblocknum))))

                                     
                                     (if (not (= (seqblock-info :seqtracknum) new-seqtracknum))

                                         ;; change track
                                         (replace-seqblock new-pos #f)

                                         (begin
                                           (define prev-pos (and (> seqblocknum 0) (<ra> :get-seqblock-start-time (1- seqblocknum) seqtracknum)))
                                           (define curr-pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                           (define next-pos (and (< seqblocknum (1- (<ra> :get-num-seqblocks seqtracknum)))
                                                                 (<ra> :get-seqblock-start-time (1+ seqblocknum) seqtracknum)))
                                           (define prev-length (and prev-pos (- (<ra> :get-seqblock-end-time (1- seqblocknum) seqtracknum)
                                                                                prev-pos)))
                                           (define curr-length (- (<ra> :get-seqblock-end-time seqblocknum seqtracknum)
                                                                  (<ra> :get-seqblock-start-time seqblocknum seqtracknum)))
                                           (define next-length (and next-pos (- (<ra> :get-seqblock-end-time (1+ seqblocknum) seqtracknum)
                                                                                next-pos)))

                                 
                                           (cond (#f #f)
                                                 
                                                 ;; Swap with previous block
                                                 ((and prev-pos
                                                       (<= new-pos
                                                           (twofifths prev-pos (+ prev-pos prev-length))))
                                                  ;;(<= new-pos prev-pos))
                                                  (let* ((new-pos2 prev-pos)
                                                         (new-pos1 (+ new-pos2 curr-length)))
                                                    (swap-blocks (1- seqblocknum) (1- seqblocknum) new-pos1 new-pos2)))
                                                 
                                                 ;; Swap with next block
                                                 ((and next-pos
                                                       (>= (+ new-pos curr-length)
                                                           (threefifths next-pos (+ next-pos next-length))))
                                                  (let* ((new-pos2 (- next-pos curr-length))
                                                         (new-pos1 (+ new-pos2 next-length)))
                                                    (swap-blocks seqblocknum (1+ seqblocknum) new-pos1 new-pos2)))

                                                 ;; Move gfx
                                                 (else
                                                  (<ra> :move-seqblock-gfx seqblocknum new-pos seqtracknum)
                                                  seqblock-info)))))

                        :Use-Place #f

                        :Mouse-pointer-func ra:set-closed-hand-mouse-pointer
                        ;;:Mouse-pointer-func ra:set-blank-mouse-pointer
                        
                        :Get-pixels-per-value-unit (lambda (seqblock-info)
                                                     (/ (- (<ra> :get-sequencer-x2)
                                                           (<ra> :get-sequencer-x1))
                                                        (- (<ra> :get-sequencer-visible-end-time)
                                                           (<ra> :get-sequencer-visible-start-time))))

                        )


(define gakkgakk-startseqtracknum 0)
(define gakkgakk-startseqblocknum 0)
(define gakkgakk-smallest-time 0)

;; Move multiple seqblocks
(add-node-mouse-handler :Get-area-box (lambda()
                                        (<ra> :get-box sequencer))
                        :Get-existing-node-info (lambda (X Y callback)                                                  
                                                  (let ((seqtracknum *current-seqtrack-num*))
                                                    (and (not *current-seqautomation/distance*)
                                                         seqtracknum
                                                         (begin
                                                           (<ra> :select-seqtrack seqtracknum)
                                                           (let ((seqblock-info (get-seqblock-info X Y)))
                                                             ;;(c-display "get-existing " seqblock-info X Y)
                                                             (and seqblock-info
                                                                  (let* ((seqtracknum (and seqblock-info (seqblock-info :seqtracknum)))
                                                                         (seqblocknum (and seqblock-info (seqblock-info :seqblocknum)))
                                                                         (is-selected (<ra> :is-seqblock-selected seqblocknum seqtracknum)))
                                                                    
                                                                    (if (not (<ra> :is-playing-song))
                                                                        (<ra> :select-block (<ra> :get-seqblock-blocknum seqblocknum seqtracknum)))
                                                                    
                                                                    (cond ((and (not is-selected)
                                                                                (not (<ra> :ctrl-pressed)))
                                                                           (only-select-one-seqblock seqblocknum seqtracknum))
                                                                          (else
                                                                           (<ra> :select-seqblock #t seqblocknum seqtracknum)))
                                                                    
                                                                    
                                                                    (cond ((> (<ra> :get-num-selected-seqblocks) 1)
                                                                           (set-grid-type #t)
                                                                           
                                                                           (set! gakkgakk-has-made-undo #f)                                                                    
                                                                           (set! gakkgakk-start-pos 0);;(<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                                                           (set! gakkgakk-was-selected is-selected)
                                                                           (set! gakkgakk-startseqtracknum seqtracknum)
                                                                           (set! gakkgakk-startseqblocknum seqblocknum)

                                                                           (define seqblock-infos (get-selected-seqblock-infos))

                                                                           (set! gakkgakk-smallest-time (apply min (map (lambda (seqblock-info)
                                                                                                                          (<ra> :get-seqblock-start-time
                                                                                                                                (seqblock-info :seqblocknum)
                                                                                                                                (seqblock-info :seqtracknum)))
                                                                                                                        seqblock-infos)))

                                                                           (create-gfx-gfx-seqblocks seqblock-infos 0 0)

                                                                           (set! gakkgakk-start-pos (list 0 Y))

                                                                           (callback seqblock-infos 0 Y))

                                                                          (else
                                                                           #f)))))))))
                        
                        :Get-min-value (lambda (seqblock-infos)
                                         -100000000000)
                                         ;;(define seqtracknum (seqblock-info :seqtracknum))
                                         ;;(define seqblocknum (seqblock-info :seqblocknum))
                                         ;;(if (or #t (= 0 seqblocknum))
                                         ;;    0
                                         ;;    (<ra> :get-seqblock-end-time (1- seqblocknum) seqtracknum)))
                        :Get-max-value (lambda (seqblock-infos)
                                         100000000000)
                                         ;;(define seqtracknum (seqblock-info :seqtracknum))
                                         ;;(define seqblocknum (seqblock-info :seqblocknum))
                                         ;;(define num-seqblocks (<ra> :get-num-seqblocks (seqblock-info :seqtracknum)))
                                         ;;(if (or #t (= (1- num-seqblocks) seqblocknum))
                                         ;;    (+ 10000000000 (<ra> :get-seqblock-end-time (seqblock-info :seqblocknum) seqtracknum))
                                         ;;    (<ra> :get-seqblock-start-time (1+ seqblocknum) seqtracknum)))
                        :Get-x (lambda (info) #f) ;;(/ (+ (<ra> :get-seqblock-x1 (info :seqblocknum)
                                                  ;;        (info :seqtracknum))
                                                  ;;  (<ra> :get-seqblock-x2 (info :seqblocknum)
                                                  ;;        (info :seqtracknum)))
                                                  ;;2))
                        :Get-y (lambda (info) #f) ;;(/ (+ (<ra> :get-seqblock-y1 (info :seqblocknum)
                                                  ;;        (info :seqtracknum))
                                                  ;;  (<ra> :get-seqblock-y2 (info :seqblocknum)
                                                  ;;        (info :seqtracknum)))
                                                  ;;2))
                        :Make-undo (lambda (_)
                                     #f)
                        :Create-new-node (lambda (X seqtracknum callback)
                                           #f)

                        :Publicize (lambda (seqblock-info)
                                     (<ra> :set-statusbar-text (two-decimal-string (/ gakkgakk-really-last-inc-time
                                                                                      (<ra> :get-sample-rate)))))

                        
                        :Release-node (lambda (seqblock-infos)
                                        (define has-moved (not (morally-equal? gakkgakk-start-pos gakkgakk-last-value)))
                                        (delete-all-gfx-gfx-seqblocks)
                                        ;;(c-display "has-moved:" has-moved gakkgakk-start-pos gakkgakk-last-value gakkgakk-was-selected)
                                        (if has-moved
                                            (apply-gfx-gfx-seqblocks seqblock-infos))

                                        (if (and (not has-moved)
                                                 (<ra> :ctrl-pressed))
                                            (<ra> :select-seqblock (not gakkgakk-was-selected) gakkgakk-startseqblocknum gakkgakk-startseqtracknum))
                                        
                                        (set-grid-type #f)

                                        seqblock-infos)

                        :Move-node (lambda (seqblock-infos Value Y)
                                     (set! gakkgakk-last-value (list Value Y))

                                     (define new-seqtracknum (or (get-seqtracknum (1+ (<ra> :get-seqtrack-x1 0)) Y)
                                                                 (if (<= Y (<ra> :get-seqtrack-y1 0))
                                                                     0
                                                                     (<ra> :get-num-seqtracks))))
                                     (define inctrack (- new-seqtracknum gakkgakk-startseqtracknum))
                                     ;;(c-display new-seqtracknum gakkgakk-startseqtracknum inctrack)
                                     (delete-all-gfx-gfx-seqblocks)
                                     (create-gfx-gfx-seqblocks seqblock-infos (floor Value) inctrack)
                                     seqblock-infos)
                                          
                        :Use-Place #f

                        :Mouse-pointer-func ra:set-closed-hand-mouse-pointer
                        ;;:Mouse-pointer-func ra:set-blank-mouse-pointer
                        
                        :Get-pixels-per-value-unit (lambda (seqblock-infos)
                                                     (/ (- (<ra> :get-sequencer-x2)
                                                           (<ra> :get-sequencer-x1))
                                                        (- (<ra> :get-sequencer-visible-end-time)
                                                           (<ra> :get-sequencer-visible-start-time))))

                        )

;; selection rectangle
(add-mouse-cycle
 (let* ((*selection-rectangle-start-x* #f)
        (*selection-rectangle-start-y* #f))
   (define (set-rect! $x $y)
     (define min-x (min $x *selection-rectangle-start-x*))
     (define min-y (min $y *selection-rectangle-start-y*))
     (define max-x (max $x *selection-rectangle-start-x*))
     (define max-y (max $y *selection-rectangle-start-y*))
     ;;(c-display min-x min-y max-x max-y)
     (<ra> :set-sequencer-selection-rectangle min-x min-y max-x max-y))

 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                ;;(c-display "in-sequencer: " (inside-box (<ra> :get-box sequencer) $x $y) (< $y (<ra> :get-seqnav-y1)))
                (and (= $button *left-button*)
                     (inside-box (<ra> :get-box sequencer) $x $y)
                     (< $y (<ra> :get-seqnav-y1))
                     (begin
                       (set! *selection-rectangle-start-x* $x)
                       (set! *selection-rectangle-start-y* $y)
                       #t)))

  :drag-func  (lambda ($button $x $y)
                (set-rect! $x $y))

  :release-func (lambda ($button $x $y)
                  (set-rect! $x $y)
                  (<ra> :unset-sequencer-selection-rectangle)))))




;; sequencer track automation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define-struct seqautomation/distance
  :seqtrack
  :seqautomation
  :distance)

(define *seqnode-min-distance* (* 1 (<ra> :get-half-of-node-width)))

(define *current-seqautomation/distance* #f)

(define (get-current-seqautomationnum)
  (and *current-seqautomation/distance*
       (*current-seqautomation/distance* :seqautomation)))
(define (get-current-seqautomation-distance)
  (and *current-seqautomation/distance*
       (*current-seqautomation/distance* :distance)))
  
  
(define (min-seqautomation/distance A B)
  (cond ((not A)
         B)
        ((not B)
         A)
        ((<= (A :distance) (B :distance))
         A)
        (else
         B)))

(define (get-closest-seqautomation-1 Nodenum Total-Nodes Seqautomation X Y X1 Y1 X2 Y2)
  (define this (and (>= X (- X1 *seqnode-min-distance*))
                    (<= X (+ X2 *seqnode-min-distance*))
                    (make-seqautomation/distance :seqtrack *current-seqtrack-num*
                                                 :seqautomation Seqautomation
                                                 :distance (let ((dist (get-distance-vertical X Y X1 Y1 X2 Y2)))
                                                             ;;(c-display "dist" dist)
                                                             dist))))

  (define next (and (< Nodenum Total-Nodes)
                    (get-closest-seqautomation-1 (1+ Nodenum)
                                                 Total-Nodes
                                                 Seqautomation
                                                 X Y
                                                 X2 Y2
                                                 (<ra> :get-seq-automation-node-x Nodenum Seqautomation *current-seqtrack-num*)
                                                 (<ra> :get-seq-automation-node-y Nodenum Seqautomation *current-seqtrack-num*))))
  (min-seqautomation/distance this
                              next))

(define-match get-closest-seqautomation-0
  Seqautomation Seqautomation        _ _ :> #f
  Seqautomation Total-Seqautomations X Y :> (min-seqautomation/distance (get-closest-seqautomation-1 2
                                                                                                     (<ra> :get-num-seq-automation-nodes Seqautomation *current-seqtrack-num*)
                                                                                                     Seqautomation
                                                                                                     X Y
                                                                                                     (<ra> :get-seq-automation-node-x 0 Seqautomation *current-seqtrack-num*)
                                                                                                     (<ra> :get-seq-automation-node-y 0 Seqautomation *current-seqtrack-num*)
                                                                                                     (<ra> :get-seq-automation-node-x 1 Seqautomation *current-seqtrack-num*)
                                                                                                     (<ra> :get-seq-automation-node-y 1 Seqautomation *current-seqtrack-num*))
                                                                        (get-closest-seqautomation-0 (1+ Seqautomation)
                                                                                                     Total-Seqautomations
                                                                                                     X
                                                                                                     Y)))


(define (get-closest-seqautomation X Y)
  (and *current-seqtrack-num*
       (get-closest-seqautomation-0 0 (<ra> :get-num-seq-automations *current-seqtrack-num*) X Y)))


#||
(get-closest-seqautomation (<ra> :get-mouse-pointer-x) (<ra> :get-mouse-pointer-y))
||#

(define (set-seqnode-statusbar-text Num)
  (let* ((automationnum (*current-seqautomation/distance* :seqautomation))
         (seqtracknum (*current-seqautomation/distance* :seqtrack))
         (instrument-id (<ra> :get-seq-automation-instrument-id automationnum seqtracknum))
         ;;(instrument-name (<ra> :get-instrument-name instrument-id))
         (effect-num (<ra> :get-seq-automation-effect-num automationnum seqtracknum))
         (effect-name (<ra> :get-instrument-effect-name effect-num instrument-id)))
    (<ra> :set-statusbar-text (<-> effect-name ": "
                                   (two-decimal-string (<ra> :get-seq-automation-value Num automationnum seqtracknum))))))
  
;; Highlight current seq automation
(add-mouse-move-handler
 :move (lambda (Button X Y)
         (let ((curr-dist (get-closest-seqautomation X Y)))
           (set! *current-seqautomation/distance* (and curr-dist
                                                       (< (curr-dist :distance) *seqnode-min-distance*)
                                                       curr-dist))
           (if *current-seqautomation/distance*
               (let* ((automationnum (*current-seqautomation/distance* :seqautomation))
                      (seqtracknum (*current-seqautomation/distance* :seqtrack))
                      (instrument-id (<ra> :get-seq-automation-instrument-id automationnum seqtracknum))
                      (instrument-name (<ra> :get-instrument-name instrument-id))
                      (effect-num (<ra> :get-seq-automation-effect-num automationnum seqtracknum))
                      (effect-name (<ra> :get-instrument-effect-name effect-num instrument-id)))
                 (<ra> :set-normal-mouse-pointer)
                 (<ra> :set-statusbar-text (<-> instrument-name "/" effect-name))
                 (<ra> :set-curr-seq-automation (*current-seqautomation/distance* :seqautomation)
                                                (*current-seqautomation/distance* :seqtrack)))
               (<ra> :cancel-curr-seq-automation)))))



;; move and create
(add-node-mouse-handler :Get-area-box (lambda ()
                                        (and *current-seqautomation/distance*
                                             (<ra> :get-box sequencer)))

                        :Get-existing-node-info (lambda (X Y callback)
                                                  (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                                                        (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                                                    (define (get-nodebox $num)
                                                      (get-common-node-box (<ra> :get-seq-automation-node-x $num automationnum seqtracknum)
                                                                           (<ra> :get-seq-automation-node-y $num automationnum seqtracknum)))
                                                    (match (list (find-node-horizontal X Y get-nodebox (<ra> :get-num-seq-automation-nodes automationnum seqtracknum)))
                                                           (existing-box Num Box) :> (begin
                                                                                       (define Time (scale X
                                                                                                           (<ra> :get-seqtrack-x1 seqtracknum) (<ra> :get-seqtrack-x2 seqtracknum)
                                                                                                           (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
                                                                                       (set-grid-type #t)
                                                                                       (callback Num Time Y))
                                                           _                      :> #f)))
                        :Get-min-value (lambda (_)
                                         (<ra> :get-sequencer-visible-start-time))
                        :Get-max-value (lambda (_)
                                         (<ra> :get-sequencer-visible-end-time))
                        :Get-x (lambda (Num)
                                 (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                                       (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                                   (<ra> :get-seq-automation-node-x Num automationnum seqtracknum)))
                        :Get-y (lambda (Num)
                                 (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                                       (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                                   (<ra> :get-seq-automation-node-y Num automationnum seqtracknum)))
                        :Make-undo (lambda (_)
                                     (<ra> :undo-sequencer))
                        :Create-new-node (lambda (X Y callback)
                                           (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                                                 (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                                             (define Time (scale X
                                                                 (<ra> :get-seqtrack-x1 seqtracknum) (<ra> :get-seqtrack-x2 seqtracknum)
                                                                 (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
                                             (define PositionTime (if (<ra> :ctrl-pressed)
                                                                      Time
                                                                      (<ra> :find-closest-seqtrack-beat-start 0 (floor Time))))
                                             (define Value (scale Y (<ra> :get-seqtrack-y1 seqtracknum) (<ra> :get-seqtrack-y2 seqtracknum) 1 0))
                                             (define Num (<ra> :add-seq-automation-node PositionTime Value *logtype-linear* automationnum seqtracknum))
                                             (if (= -1 Num)
                                               #f
                                               (begin
                                                 (set-grid-type #t)
                                                 (callback Num Time)))))
                        :Release-node (lambda (Num)
                                        (set-grid-type #f))
                        :Move-node (lambda (Num Time Y)
                                     (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                                           (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                                       (define Value (scale Y (<ra> :get-seqtrack-y1 seqtracknum) (<ra> :get-seqtrack-y2 seqtracknum) 1 0))
                                       (define logtype (<ra> :get-seq-automation-logtype Num automationnum seqtracknum))
                                       (set! Time (floor Time))
                                       (if (not (<ra> :ctrl-pressed))
                                           (set! Time (<ra> :find-closest-seqtrack-beat-start 0 Time)))
                                       (<ra> :set-seq-automation-node Time Value logtype Num automationnum seqtracknum)
                                       ;;(c-display "NUM:" Num ", Time:" Time ", Value:" Value)
                                       Num))
                        :Publicize (lambda (Num)
                                     (set-seqnode-statusbar-text Num)
                                     ;;(<ra> :set-curr-seqtemponode Num)
                                     #f)
                        
                        :Use-Place #f
                        :Mouse-pointer-func ra:set-normal-mouse-pointer
                        :Get-pixels-per-value-unit #f
                        )         



;; delete seqautomation / popupmenu
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda ($button $x $y)
                (and (= $button *right-button*)
                     *current-seqautomation/distance*
                     (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                           (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                       (define (get-nodebox $num)
                         (get-common-node-box (<ra> :get-seq-automation-node-x $num automationnum seqtracknum)
                                              (<ra> :get-seq-automation-node-y $num automationnum seqtracknum)))
                       (define Num (match (list (find-node-horizontal $x $y get-nodebox (<ra> :get-num-seq-automation-nodes automationnum seqtracknum)))
                                          (existing-box Num Box) :> Num
                                          A                      :> #f))
                       (if (<ra> :shift-pressed)
                           (<ra> :delete-seq-automation-node Num automationnum seqtracknum)
                           (popup-menu (list "Delete"
                                             :enabled Num
                                             (lambda ()
                                               (<ra> :delete-seq-automation-node Num automationnum seqtracknum)))
                                       ;;(list "Reset (set value to 0.5)"
                                       ;;     :enabled Num
                                       ;;      (lambda ()
                                       ;;        (<ra> :undo-seqtempo)
                                       ;;        (<ra> :set-seqtemponode
                                       ;;              (<ra> :get-seqtempo-abstime Num)
                                       ;;              1.0
                                       ;;              (<ra> :get-seqtempo-logtype Num)
                                       ;;              Num)))
                                       (list "Glide to next break point"
                                             :check (and Num (= (<ra> :get-seq-automation-logtype Num automationnum seqtracknum)
                                                                *logtype-linear*))
                                             :enabled Num
                                             (lambda (maybe)
                                               (<ra> :undo-sequencer)
                                               (<ra> :set-seq-automation-node
                                                     (<ra> :get-seq-automation-time Num automationnum seqtracknum)
                                                     (<ra> :get-seq-automation-value Num automationnum seqtracknum)
                                                     (if maybe *logtype-linear* *logtype-hold*)
                                                     Num
                                                     automationnum
                                                     seqtracknum)))
                                       ))
                       #t)))))

;; highlight current seq automation node
(add-mouse-move-handler
 :move (lambda ($button $x $y)
         (and *current-seqautomation/distance*
              (let ((automationnum (*current-seqautomation/distance* :seqautomation))
                    (seqtracknum (*current-seqautomation/distance* :seqtrack)))
                (define (get-nodebox $num)
                  (get-common-node-box (<ra> :get-seq-automation-node-x $num automationnum seqtracknum)
                                       (<ra> :get-seq-automation-node-y $num automationnum seqtracknum)))
                (match (list (find-node-horizontal $x $y get-nodebox (<ra> :get-num-seq-automation-nodes automationnum seqtracknum)))
                       (existing-box Num Box) :> (begin
                                                   (set-seqnode-statusbar-text Num)
                                                   (<ra> :set-curr-seq-automation-node Num automationnum seqtracknum)
                                                   #t)
                       A                      :> (begin
                                                   ;;(c-display "**Didnt get it:" A)
                                                   (<ra> :cancel-curr-seq-automation-node automationnum seqtracknum)
                                                   #f))))))


 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;; delete seqblock
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     (<ra> :shift-pressed)
                     (let ((seqblock-info (get-seqblock-info X Y)))
                       ;;(c-display "get-existing " seqblock-info X Y)
                       (and seqblock-info
                            (begin
                              (<ra> :delete-seqblock (seqblock-info :seqblocknum) (seqblock-info :seqtracknum))
                              #t)))))))

(define (get-sequencer-pos-from-x X)
  (round (scale X
                (<ra> :get-sequencer-x1) (<ra> :get-sequencer-x2)
                (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time))))


(define *seqblock-clipboard* '())

(define-struct clipboard-seqblock
  :seqtracknum
  :blocknum
  :time
  )

(define (copy-all-selected-seqblocks)
  (define minseqtrack #f)
  (define mintime #f)
  (set! *seqblock-clipboard* '())

  (define (add-seqblock! seqtracknum seqblocknum)
    (define time (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
    (push-back! *seqblock-clipboard*
                (make-clipboard-seqblock :seqtracknum seqtracknum
                                         :blocknum (<ra> :get-seqblock-blocknum seqblocknum seqtracknum)
                                         :time time))
    (if (not minseqtrack)
        (set! minseqtrack seqtracknum)
        (set! minseqtrack (min seqtracknum minseqtrack)))
    (if (not mintime)
        (set! mintime time)
        (set! mintime (min time mintime))))

  ;; Find all selected seqblocks
  (for-each-seqblock (lambda (seqtracknum seqblocknum)
                       (when (<ra> :is-seqblock-selected seqblocknum seqtracknum)
                         (add-seqblock! seqtracknum seqblocknum))))

  (when (null? *seqblock-clipboard*)
    (define x (<ra> :get-mouse-pointer-x))
    (define y (<ra> :get-mouse-pointer-y))
    (for-each-seqblock (lambda (seqtracknum seqblocknum)
                         (if (inside-box (<ra> :get-box seqblock seqblocknum seqtracknum) x y)
                             (add-seqblock! seqtracknum seqblocknum)))))

  ;; Scale time
  (set! *seqblock-clipboard*
        (map (lambda (clipboard-seqblock)
               (make-clipboard-seqblock :seqtracknum (- (clipboard-seqblock :seqtracknum) minseqtrack)
                                        :blocknum (clipboard-seqblock :blocknum)
                                        :time (- (clipboard-seqblock :time) mintime)))
             *seqblock-clipboard*)))

(define (paste-sequencer-blocks seqtracknum time)
  (undo-block
   (lambda ()
     (for-each (lambda (clipboard-seqblock)
                 (define seqtracknum2 (+ seqtracknum (clipboard-seqblock :seqtracknum)))
                 (if (< seqtracknum2 (<ra> :get-num-seqtracks))
                     (<ra> :add-block-to-seqtrack
                           seqtracknum2
                           (clipboard-seqblock :blocknum)
                           (+ time (clipboard-seqblock :time)))))
               *seqblock-clipboard*))))

(define (delete-all-selected-seqblocks)
  (define deleted-something #f)

  (undo-block
   (lambda ()
     (let loop ((seqblocknum 0)
                (seqtracknum 0))
       (cond ((= seqtracknum (<ra> :get-num-seqtracks))
              #t)
             ((= seqblocknum (<ra> :get-num-seqblocks seqtracknum))
              (loop 0 (1+ seqtracknum)))
             ((<ra> :is-seqblock-selected seqblocknum seqtracknum)
              (<ra> :delete-seqblock seqblocknum seqtracknum)
              (set! deleted-something #t)
              (loop seqblocknum seqtracknum))
             (else
              (loop (1+ seqblocknum) seqtracknum))))
     
     (when (not deleted-something)
       (define x (<ra> :get-mouse-pointer-x))
       (define y (<ra> :get-mouse-pointer-y))
       (let loop ((seqblocknum 0)
                  (seqtracknum 0))
         (cond ((= seqtracknum (<ra> :get-num-seqtracks))
                #t)
               ((= seqblocknum (<ra> :get-num-seqblocks seqtracknum))
                (loop 0 (1+ seqtracknum)))
               ((inside-box (<ra> :get-box seqblock seqblocknum seqtracknum) x y)
                (<ra> :delete-seqblock seqblocknum seqtracknum)
                (loop seqblocknum seqtracknum))
               (else
                (loop (1+ seqblocknum) seqtracknum))))))))

(define (cut-all-selected-seqblocks)
  (copy-all-selected-seqblocks)
  (delete-all-selected-seqblocks))


(define (create-sequencer-automation seqtracknum X Y)
  (define all-instruments (get-all-audio-instruments))
  (popup-menu
   (map (lambda (num instrument-id)
          (list (<-> num ". " (<ra> :get-instrument-name instrument-id))
                (lambda ()
                  (popup-menu (map (lambda (effectnum)
                                     (list (<-> effectnum ". " (<ra> :get-instrument-effect-name effectnum instrument-id))
                                           (lambda ()
                                             (define Time1 (scale X
                                                                 (<ra> :get-seqtrack-x1 seqtracknum) (<ra> :get-seqtrack-x2 seqtracknum)
                                                                 (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
                                             (define Time2 (scale (+ X (* 5 *seqnode-min-distance*))
                                                                  (<ra> :get-seqtrack-x1 seqtracknum) (<ra> :get-seqtrack-x2 seqtracknum)
                                                                  (<ra> :get-sequencer-visible-start-time) (<ra> :get-sequencer-visible-end-time)))
                                             (define Value (scale Y (<ra> :get-seqtrack-y1 seqtracknum) (<ra> :get-seqtrack-y2 seqtracknum) 1 0))
                                             (c-display num effectnum)
                                             (<ra> :add-seq-automation
                                                   (floor Time1) Value
                                                   (floor Time2) Value
                                                   effectnum
                                                   instrument-id
                                                   seqtracknum))))
                                   (iota (<ra> :get-num-instrument-effects instrument-id)))))))
        (iota (length all-instruments))
        all-instruments)))


;; seqblock menu
(add-mouse-cycle
 (make-mouse-cycle
  :press-func (lambda (Button X Y)
                (and (= Button *right-button*)
                     (not (<ra> :shift-pressed))
                     (let ((seqtracknum *current-seqtrack-num*))
                       (and seqtracknum
                            (let ((seqblock-info (get-seqblock-info X Y)))
                              (define seqblocknum (and seqblock-info
                                                       (seqblock-info :seqblocknum)))
                              (define blocknum (and seqblock-info
                                                    (<ra> :get-seqblock-blocknum seqblocknum
                                                          seqtracknum)))
                              (if seqblock-info
                                  (if (not (<ra> :is-seqblock-selected seqblocknum seqtracknum))
                                      (only-select-one-seqblock seqblocknum seqtracknum)
                                      (<ra> :select-seqblock #t seqblocknum seqtracknum)))

                              (popup-menu "Insert existing block" (lambda ()
                                                                    (let ((pos (<ra> :find-closest-seqtrack-bar-start seqtracknum (get-sequencer-pos-from-x X))))
                                                                      (apply popup-menu
                                                                             (map (lambda (blocknum)
                                                                                    (list (<-> blocknum ": " (<ra> :get-block-name blocknum))
                                                                                          (lambda ()
                                                                                            (<ra> :add-block-to-seqtrack seqtracknum blocknum pos))))
                                                                                  (iota (<ra> :get-num-blocks))))))

                                          ;;   Sub menues version. It looks better, but it is less convenient.
                                          ;;"Insert existing block" (map (lambda (blocknum)
                                          ;;                               (list (<-> blocknum ": " (<ra> :get-block-name blocknum))
                                          ;;                                     (lambda ()
                                          ;;                                       (let ((pos (get-sequencer-pos-from-x X)))
                                          ;;                                         (<ra> :add-block-to-seqtrack seqtracknum blocknum pos))
                                          ;;                                       (<ra> :select-block blocknum))))
                                          ;;                             (iota (<ra> :get-num-blocks)))
                                          
                                          "Insert current block" (lambda ()
                                                                   (let* ((pos (<ra> :find-closest-seqtrack-bar-start seqtracknum (get-sequencer-pos-from-x X)))
                                                                          (blocknum (<ra> :current-block)))
                                                                     (<ra> :add-block-to-seqtrack seqtracknum blocknum pos)))
                                          
                                          "Insert new block" (lambda ()
                                                               (let* ((pos (<ra> :find-closest-seqtrack-bar-start seqtracknum (get-sequencer-pos-from-x X)))
                                                                      (blocknum (<ra> :append-block)))
                                                                 (<ra> :add-block-to-seqtrack seqtracknum blocknum pos)))

                                          "--------------------"
                                          
                                          (list (if (> (<ra> :get-num-selected-seqblocks) 1)
                                                    "Copy sequencer blocks"
                                                    "Copy sequencer block")
                                                :enabled seqblock-info
                                                ra:copy-selected-seqblocks)
                                          
                                          (list (if (> (<ra> :get-num-selected-seqblocks) 1)
                                                    "Cut sequencer blocks"
                                                    "Cut sequencer block")
                                                :enabled seqblock-info
                                                ra:cut-selected-seqblocks)
                                          
                                          (list (if (> (<ra> :get-num-selected-seqblocks) 1)
                                                    "Delete sequencer blocks"
                                                    "Delete sequencer block")
                                                :enabled seqblock-info
                                                ra:delete-selected-seqblocks)
                                          
                                          (list (if (> (<ra> :get-num-selected-seqblocks) 1)
                                                    "Paste sequencer blocks"
                                                    "Paste sequencer block")
                                                :enabled (not (empty? *seqblock-clipboard*))
                                                (lambda ()
                                                  (let ((pos (<ra> :find-closest-seqtrack-bar-start seqtracknum (get-sequencer-pos-from-x X))))
                                                    (<ra> :paste-seqblocks seqtracknum pos))))
                                          
                                          
                                          "--------------------"
                                          
                                          ;;(list "Replace with current block"
                                          ;;      :enabled seqblock-info
                                          ;;      (lambda ()
                                          ;;        (undo-block
                                          ;;         (lambda ()
                                          ;;           (define pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                          ;;           (<ra> :delete-seqblock seqblocknum seqtracknum)                 
                                          ;;           (<ra> :add-block-to-seqtrack seqtracknum (<ra> :current-block) pos)))))


                                          (list "Replace with existing block"
                                                :enabled seqblock-info
                                                (lambda ()
                                                  (apply popup-menu
                                                         (let ((pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum)))
                                                           (map (lambda (blocknum)
                                                                  (list (<-> blocknum ": " (<ra> :get-block-name blocknum))
                                                                        (lambda ()
                                                                          (undo-block
                                                                           (lambda ()
                                                                             (<ra> :delete-seqblock seqblocknum seqtracknum)
                                                                             (<ra> :add-block-to-seqtrack seqtracknum blocknum pos)))
                                                                          (<ra> :select-block blocknum))))                                                         
                                                                (iota (<ra> :get-num-blocks)))))))

                                          ;;   Sub menues version. It looks better, but it is less convenient.
                                          ;;(list "Replace with existing block"
                                          ;;      :enabled seqblock-info
                                          ;;      (if seqblock-info
                                          ;;          (let ((pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum)))
                                          ;;            (map (lambda (blocknum)
                                          ;;                   (list (<-> blocknum ": " (<ra> :get-block-name blocknum))
                                          ;;                         (lambda ()
                                          ;;                           (undo-block
                                          ;;                            (lambda ()
                                          ;;                              (<ra> :delete-seqblock seqblocknum seqtracknum)
                                          ;;                              (<ra> :add-block-to-seqtrack seqtracknum blocknum pos)))
                                          ;;                           (<ra> :select-block blocknum))))                                                         
                                          ;;                 (iota (<ra> :get-num-blocks))))
                                          ;;          (lambda ()
                                          ;;            #f)))

                                          ;; Doesn't make sense since we select block under mouse when right-clicking on it.
                                          ;;(list "Replace with current block"
                                          ;;      :enabled seqblock-info
                                          ;;      (lambda ()
                                          ;;        (let ((pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                          ;;              (blocknum (<ra> :current-block)))
                                          ;;          (undo-block
                                          ;;           (lambda ()
                                          ;;             (<ra> :delete-seqblock seqblocknum seqtracknum)
                                          ;;             (<ra> :add-block-to-seqtrack seqtracknum blocknum pos)))
                                          ;;          (<ra> :select-block blocknum))))
                                          
                                          (list "Replace with new block"
                                                :enabled seqblock-info
                                                (lambda ()
                                                  (let ((pos (<ra> :get-seqblock-start-time seqblocknum seqtracknum))
                                                        (blocknum (<ra> :append-block)))
                                                    (undo-block
                                                     (lambda ()
                                                       (<ra> :delete-seqblock seqblocknum seqtracknum)
                                                       (<ra> :add-block-to-seqtrack seqtracknum blocknum pos)))
                                                    (<ra> :select-block blocknum))))
                                          
                                          ;;"-----------------"
                                          "------------------"

                                          (list "Clone block"
                                                :enabled (and seqblock-info (not (<ra> :is-playing-song)))
                                                (lambda ()
                                                  (<ra> :select-block blocknum)
                                                  (<ra> :copy-block)
                                                  (undo-block
                                                   (lambda ()
                                                     (define new-blocknum (<ra> :append-block))
                                                     (<ra> :select-block new-blocknum)
                                                     (<ra> :paste-block)))))
                                          
                                          (list "Configure block"
                                                :enabled (and seqblock-info (not (<ra> :is-playing-song)))
                                                (lambda ()
                                                  (<ra> :select-block blocknum)
                                                  (<ra> :config-block)))
                                          
                                          (list "Configure block color"
                                                :enabled seqblock-info
                                                (lambda ()
                                                  (<ra> :color-dialog (<ra> :get-block-color blocknum)
                                                                      (lambda (color)
                                                                        (<ra> :set-block-color color blocknum)))))
                                          ;;
                                          ;;(list "Remove pause"
                                          ;;      :enabled #f
                                          ;;      (lambda ()
                                          ;;        #f))

                                          "-----------------"

                                          "New automation" (lambda ()
                                                             (create-sequencer-automation seqtracknum X Y))

                                          "-----------------"
                                          
                                          "Insert sequencer track" (lambda ()
                                                                     (<ra> :insert-seqtrack seqtracknum))
                                          (list "Delete sequencer track"
                                                :enabled (> (<ra> :get-num-seqtracks) 1)
                                                (lambda ()
                                                  (<ra> :delete-seqtrack seqtracknum)))
                                          "Append sequencer track" (lambda ()
                                                                     (<ra> :append-seqtrack))

                                          "-----------------"

                                          (list "Song tempo automation visible"
                                                :check (<ra> :seqtempo-visible)
                                                (lambda (doit)
                                                  (<ra> :set-seqtempo-visible doit)))
                                          (list "Play loop"
                                                :check (<ra> :is-seqlooping)
                                                (lambda (val)
                                                  (<ra> :set-seqlooping val)))
                                          ))))))))
                                                                                 


;; left size handle in navigator
(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (define box (<ra> :get-box seqnav-left-size-handle))
                                            ;;(c-display "box" box)
                                            (and (inside-box box X Y)
                                                 (<ra> :get-seqnav-left-size-handle-x1)))
                        :Get-x1 (lambda (_)
                                  (<ra> :get-seqnav-x1))
                        :Get-x2 (lambda (_)
                                  (1- (<ra> :get-seqnav-right-size-handle-x2)))
                        :Get-min-value (lambda (_)
                                         (<ra> :get-seqnav-x1))
                        :Get-max-value (lambda (_)
                                         (1- (<ra> :get-seqnav-right-size-handle-x2)))
                        ;;(<ra> :get-seqnav-x2))
                        ;;:Get-x (lambda (_)                                 
                        ;;         (/ (+ (<ra> :get-seqnav-left-size-handle-x1)
                        ;;               (<ra> :get-seqnav-left-size-handle-x2))
                        ;;            2))
                        :Get-value (lambda (Value)
                                     Value)
                        :Make-undo (lambda (_)
                                     50)
                        :Move (lambda (_ Value)
                                (define song-length (<ra> :get-sequencer-song-length-in-frames))
                                (define new-start-time (floor (scale Value
                                                                     (<ra> :get-seqnav-x1) (<ra> :get-seqnav-x2);; (<ra> :get-seqnav-right-size-handle-x1)
                                                                     0 song-length)))
                                ;;(c-display "       Move" Value (/ new-start-time 48000.0) "x1:" (<ra> :get-seqnav-x1) "x2:" (<ra> :get-seqnav-x2) "end:" (/ (<ra> :get-sequencer-visible-end-time) 48000.0))
                                (define end-time (<ra> :get-sequencer-visible-end-time))
                                (<ra> :set-sequencer-visible-start-time (max 0 (min (1- end-time) new-start-time))))
                        :Publicize (lambda (_)
                                     (<ra> :set-statusbar-text (<-> (two-decimal-string (/ (<ra> :get-sequencer-visible-start-time) (<ra> :get-sample-rate)))
                                                                    " -> "
                                                                    (two-decimal-string (/ (<ra> :get-sequencer-visible-end-time) (<ra> :get-sample-rate))))))
                        :Mouse-pointer-func ra:set-horizontal-resize-mouse-pointer
                        )

;; right size handle in navigator
(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (define box (<ra> :get-box seqnav-right-size-handle))
                                            ;;(c-display "box2" box)
                                            (and (inside-box box X Y)
                                                 (<ra> :get-seqnav-right-size-handle-x2)))
                        :Get-x1 (lambda (_)
                                  (1- (<ra> :get-seqnav-left-size-handle-x1)))
                        :Get-x2 (lambda (_)
                                  (<ra> :get-seqnav-x2))
                        :Get-min-value (lambda (_)
                                         (1- (<ra> :get-seqnav-left-size-handle-x1)))
                        :Get-max-value (lambda (_)
                                         (<ra> :get-seqnav-x2))
                        ;;(<ra> :get-seqnav-x2))
                        ;;:Get-x (lambda (_)                                 
                        ;;         (/ (+ (<ra> :get-seqnav-right-size-handle-x1)
                        ;;               (<ra> :get-seqnav-right-size-handle-x2))
                        ;;            2))
                        :Get-value (lambda (Value)
                                     Value)
                        :Make-undo (lambda (_)
                                     50)
                        :Move (lambda (_ Value)
                                (define song-length (<ra> :get-sequencer-song-length-in-frames))
                                (define new-end-time (floor (scale Value
                                                                   (<ra> :get-seqnav-x1) (<ra> :get-seqnav-x2);; (<ra> :get-seqnav-right-size-handle-x1)
                                                                   0 song-length)))
                                ;;(c-display "       Move" Value (/ new-start-time 48000.0) "x1:" (<ra> :get-seqnav-x1) "x2:" (<ra> :get-seqnav-x2) "end:" (/ (<ra> :get-sequencer-visible-end-time) 48000.0))
                                (define start-time (<ra> :get-sequencer-visible-start-time))
                                ;;(c-display "new-end-time:" (/ new-end-time 48000.0) Value)
                                (<ra> :set-sequencer-visible-end-time (min song-length (max (1+ start-time) new-end-time))))
                        :Publicize (lambda (_)
                                     (<ra> :set-statusbar-text (<-> (two-decimal-string (/ (<ra> :get-sequencer-visible-start-time) (<ra> :get-sample-rate)))
                                                                    " -> "
                                                                    (two-decimal-string (/ (<ra> :get-sequencer-visible-end-time) (<ra> :get-sample-rate))))))
                        :Mouse-pointer-func ra:set-horizontal-resize-mouse-pointer
                        )

;; seqtrack select
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;(add-mouse-cycle
; (make-mouse-cycle
;  :press-func (lambda (Button X Y)
;                (let ((seqtracknum (get-seqtracknum X Y)))
;                  (if seqtracknum
;                      (begin
;                        (<ra> :select-seqtrack seqtracknum)))
;                  #f))))


(define (get-seqnav-width)
  (define space-left (- (<ra> :get-seqnav-left-size-handle-x1)
                        (<ra> :get-seqnav-x1)))
  (define space-right (- (<ra> :get-seqnav-x2)
                         (<ra> :get-seqnav-right-size-handle-x2)))
  (+ space-left space-right))

(define (get-seqnav-move-box)
  (make-box2 (<ra> :get-seqnav-left-size-handle-x2) (<ra> :get-seqnav-left-size-handle-y1)
             (<ra> :get-seqnav-right-size-handle-x1) (<ra> :get-seqnav-right-size-handle-y2)))


;; move navigator left/right
(add-horizontal-handler :Get-handler-data (lambda (X Y)
                                            (if (> (get-seqnav-width) 0)
                                                (let ((box (get-seqnav-move-box)))
                                                  (and (inside-box box X Y)
                                                       (<ra> :get-seqnav-left-size-handle-x1)))
                                                #f))
                        :Get-x1 (lambda (_)
                                  0)
                        :Get-x2 (lambda (_)
                                  (get-seqnav-width))
                        :Get-min-value (lambda (_)
                                         0)
                        :Get-max-value (lambda (_)
                                         (get-seqnav-width))
                        ;;:Get-x (lambda (_)
                        ;;         (+ org-x (- (<ra> :get-seqnav-left-size-handle-x1)
                        ;;                     org-seqnav-left)))
;                                 (/ (+ (<ra> :get-seqnav-left-size-handle-x1)
;                                       (<ra> :get-seqnav-right-size-handle-x2))
;                                    2))
                        :Get-value (lambda (Value)
                                     Value)
                        :Make-undo (lambda (_)
                                     50)
                        :Move (lambda (_ Value)
                                (define old-start-time (<ra> :get-sequencer-visible-start-time))
                                (define song-length (<ra> :get-sequencer-song-length-in-frames))
                                (define new-start-time (floor (scale Value
                                                                     (<ra> :get-seqnav-x1) (<ra> :get-seqnav-x2);; (<ra> :get-seqnav-right-size-handle-x1)
                                                                     0 song-length)))
                                ;;(c-display "       Move" Value (/ new-start-time 48000.0) "x1:" (<ra> :get-seqnav-x1) "x2:" (<ra> :get-seqnav-x2) "end:" (/ (<ra> :get-sequencer-visible-end-time) 48000.0))
                                (define end-time (<ra> :get-sequencer-visible-end-time))
                                (define new-start-time2 (max 0 (min (1- end-time) new-start-time)))
                                
                                (define diff (- new-start-time2 old-start-time))
                                (define new-end-time (+ end-time diff))
                                (define new-end-time2 (min song-length (max (1+ new-start-time2) new-end-time)))
                        
                                (<ra> :set-sequencer-visible-start-time new-start-time2)
                                (<ra> :set-sequencer-visible-end-time new-end-time2))
                                
                        :Publicize (lambda (_)
                                     (<ra> :set-statusbar-text (<-> (two-decimal-string (/ (<ra> :get-sequencer-visible-start-time) (<ra> :get-sample-rate)))
                                                                    " -> "
                                                                    (two-decimal-string (/ (<ra> :get-sequencer-visible-end-time) (<ra> :get-sample-rate))))))
                        
                        :Mouse-pointer-func ra:set-closed-hand-mouse-pointer
                        )





#||
(load "lint.scm")
(define *report-unused-parameters* #f)
(define *report-unused-top-level-functions* #t)
(define *report-multiply-defined-top-level-functions* #f) ; same name defined at top level in more than one file
(define *report-shadowed-variables* #t)
(define *report-minor-stuff* #t)                          ; let*, docstring checks, (= 1.5 x), numerical and boolean simplification
(lint "/home/kjetil/radium/bin/scheme/mouse/mouse.scm")

(c-display (<ra> :add-temponode 2.1 -5.0))

(box-to-string (find-temponode 210 1210))

(<ra> :set-temponode 1 65/2 8.0)
(<ra> :set-temponode 1 0.01 8.0)
(<ra> :set-temponode 3 100.01 8.0)

(<ra> :ctrl-pressed)

(define (mouse-press button x* y*)
  (if (not curr-mouse-cycle)
      (set! curr-mouse-cycle (get-mouse-cycle button x* y*))))

(define (mouse-drag button x* y*)
  (if curr-mouse-cycle
      ((cadr curr-mouse-cycle) button x* y*)))

(define (mouse-release button x* y*)
  (let ((mouse-cycle curr-mouse-cycle))
    (set! curr-mouse-cycle #f)
    (if mouse-cycle
        ((caddr mouse-cycle) button x* y*))))


||#



#||

;; testing backtracing. Haven't been able to get longer backtrace than 1/2, plus that there is only line number for the last place.
(define (d)
  (e))
(define (c)
  (d))
(define (b)
  (c))
(define (a)
  (b))
(a)

(set! (*s7* 'undefined-identifier-warnings) #t)
(*s7* 'undefined-identifier-warnings)

(define (happ2)
  (+ 2 aiai6))

(*s7* 'symbol-table)
(*s7* 'rootlet-size)

(define (get-func)
  (define (get-inner)
    __func__)
  (get-inner))

(<-> "name: " (symbol->string (get-func)) )




(begin *stacktrace*)
(a)
(stacktrace)
(set! (*s7* 'maximum-stack-size) 3134251345)
(set! (*s7* 'max-frames) 30)
(*s7*)

(let ((x 1))
  (catch #t
	 (lambda ()
           (set! (*s7* 'stack-size) 50)
	   (let ((y 2))
             (a)))
	 (lambda args
           (c-display "stacktrace" (stacktrace 5) "2")
           (c-display "args" args)
           (c-display "owlet" (owlet))
           (with-let (owlet)
                     (c-display "stack-top" (pretty-print (*s7* 'stack))))
           )))
(load "/home/kjetil/radium3.0/bin/scheme/mouse/bug.scm")

(<ra> :move-mouse-pointer 50 50)

(<ra> :append-seqtrack)
(<ra> :select-seqtrack 0)
(<ra> :select-seqtrack 1)
(<ra> :select-seqtrack 2)

(box-to-string (ra:get-box2 seqtrack 0))
(box-to-string (ra:get-box2 seqtrack 1))

(let ((time1 5.0)
      (time2 8.0)
      (val1 0.8)
      (val2 20.1))
  (scale 1.0 val1 val2 time1 time2))

(let ((time1 5.0)
      (time2 8.0)
      (val1 2.1)
      (val2 0.8))
  (scale 1.0 val1 val2 time1 time2))


||#

