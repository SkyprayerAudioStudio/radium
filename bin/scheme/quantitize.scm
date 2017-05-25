(provide 'quantitize.scm)

(define *curr-quantitize-gui* #f)

(delafina (quantitize-note :start
                           :end
                           :q
                           :max-length
                           :type 3 ;; See GUI. Type 1 is "Move start position ...", type 2 is "Move end ...", etc.
                           )

  (define delete-it #f)

  (define quantitize-start (if *curr-quantitize-gui*
                               (<gui> :get-value (<gui> :child *curr-quantitize-gui* "quant_start"))
                               #t))

  (define quantitize-end (if *curr-quantitize-gui*
                             (<gui> :get-value (<gui> :child *curr-quantitize-gui* "quant_end"))
                             #t))

  (define keep-note-length (if *curr-quantitize-gui*
                               (<gui> :get-value (<gui> :child *curr-quantitize-gui* "keep_length"))
                               #f))

  (define new-start (if quantitize-start
                        (quantitize start q)
                        start))

  (define new-end (if quantitize-end
                      (quantitize end q)
                      end))
  
  (define org-length (- end start))

  (define (keep-original-note-length!)
    (begin
      (if quantitize-start
          (set! new-end (+ new-start org-length)))
      (if quantitize-end
          (set! new-start (- new-end org-length)))))

  (if (<= q 0)
      (error 'illegal-quantitize-value q))

  (c-display "*** Calling quantitizenote" start end ", len:" org-length ", q:" q ", quant start:" quantitize-start ", quant end:" quantitize-end ", keep-note-length:" keep-note-length ", type:" type)
  
  (if keep-note-length
      (keep-original-note-length!))

  (define (legalize-length!)
    (c-display "calling legalize-length? " (>= new-start new-end))
    (if (>= new-start new-end)
        (cond ((= type 1) ;; move-start-to-previous
               (set! new-start (quantitize new-start q))
               (while (>= new-start new-end)
                 (set! new-start (- new-start q))))
              
              ((= type 2)
               (set! new-end (quantitize new-end q))
               (while (>= new-start new-end)
                 (set! new-end (+ new-end q))))
              
              ((= type 3)
               (keep-original-note-length!))
              
              ((= type 4)
               (set! new-end end)
               (while (>= new-start new-end)
                 (set! new-end (+ new-end q))))
              
              ((= type 5)
               (set! delete-it #t)))))


  (define (legal-pos pos)
    (cond ((< pos 0)
           0)
          ((> pos max-length)
           max-length)
          (else
           pos)))

  (define (legalize!)
    (legalize-length!)
    (set! new-start (legal-pos new-start))
    (set! new-end (legal-pos new-end)))

  (c-display "bef: new-start/new-end" new-start new-end ", org-len:" org-length)
  ;;(keep-original-note-length!)
  ;;(c-display "bef2: new-start/new-end" new-start new-end)

  (c-display "type: " type ", empty:" (>= new-start new-end))
  
  (legalize!)

  (if (>= new-start new-end)
      (legalize!))
  
  (if (>= new-start new-end)
      (legalize!))

  (c-display "aft: new-start/new-end" new-start new-end)
  
  (if delete-it
      #f
      (cons new-start new-end))
  )


         

(define (create-quantitize-gui)
  (define quant-gui (<gui> :ui "quantization.ui"))

  (define (set-me-as-current!)
    (set! *curr-quantitize-gui* quant-gui))

  
  ;; Quantitize Options
  ;;
  (define quant-type-range (integer-range 1 5))
  
  (define quant-type-guis (map (lambda (n)
                                 (<gui> :child quant-gui (<-> "type" n)))
                               quant-type-range))

  (<gui> :set-value (quant-type-guis (1- (<ra> :get-quantitize-type))) #t)
  (for-each (lambda (type-gui n)
              (<ra> :schedule 1000
                    (lambda ()
                      (if (<gui> :is-open type-gui)
                          (begin
                            (if (and (= (<ra> :get-quantitize-type)
                                        n)
                                     (not (<gui> :get-value type-gui)))
                                (<gui> :set-value type-gui #t))                                
                            (+ 400 (random 100)))
                          #f)))
              (<gui> :add-callback type-gui
                     (lambda (is-on)
                       (when is-on
                         (<ra> :set-quantitize-type n)))))
            quant-type-guis
            quant-type-range)

  
  ;; Quantitize value 
  ;;
  (define value-gui (<gui> :child quant-gui "quantization_value"))

  (<gui> :set-value value-gui (<ra> :get-quantitize))

  (<ra> :schedule 1000
        (lambda ()
          (if (<gui> :is-open value-gui)
              (begin
                (if (not (= (<gui> :get-value value-gui)
                            (<ra> :get-quantitize)))
                    (<gui> :set-value value-gui (<ra> :get-quantitize)))                    
                (+ 400 (random 100)))
              #f)))
           
  (<gui> :add-callback value-gui
         (lambda (val)
           (c-display "Quant: " val (string? val))
           (<ra> :set-quantitize val)))


  ;; Buttons
  ;;
  (<gui> :add-callback (<gui> :child quant-gui "quantitize_range")
         (lambda ()
           (set-me-as-current!)
           (<ra> :quantitize-range)))
  
  (<gui> :add-callback (<gui> :child quant-gui "quantitize_track")
         (lambda ()
           (set-me-as-current!)
           (<ra> :quantitize-track)))
  
  (<gui> :add-callback (<gui> :child quant-gui "quantitize_block")
         (lambda ()
           (set-me-as-current!)
           (<ra> :quantitize-block)))
  
  
  ;; Set me as current quantitize gui, and return me
  ;;
  (set-me-as-current!)
  quant-gui)


#!!
(load "notem.scm")
(add-notem-tab "Quantization" (create-quantitize-gui))
!!#

