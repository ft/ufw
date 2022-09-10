;; Copyright (c) 2022 ufw workers, All rights reserved.
;;
;; Terms for redistribution and use can be found in LICENCE.

(define-module (ufw utilities)
  #:export (!
            comma-list
            stringify))

(define (stringify obj)
  (cond ((list? obj) (map stringify obj))
        ((symbol? obj) (symbol->string obj))
        (else obj)))

(define-syntax-rule (! arg ...)
  (stringify '(arg ...)))

(define* (comma-list lst #:optional final-comma?)
  (let ((n (length lst))
        (joined (string-join lst ",")))
    (cond ((zero? n) "")
          (final-comma? (string-append joined ","))
          (else joined))))
