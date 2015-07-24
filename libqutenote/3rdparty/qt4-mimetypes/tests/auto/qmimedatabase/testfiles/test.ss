;; tested with Guile; might need
;; slight modification for your
;; Scheme dialect

(use-modules (ice-9 format))

(define (fac n)
  (let loop ((acc 1)
             (i   n))
    (if (zero? i)
        acc
        (loop (* acc i) (- i 1)))))

(format #t "(fac 5) = ~d\n" (fac 5))

