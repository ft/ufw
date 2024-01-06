;; Copyright (c) 2022-2024 ufw workers, All rights reserved.
;;
;; Terms for redistribution and use can be found in LICENCE.

(define-module (ufw register-table)
  #:use-module (ice-9 match)
  #:use-module (srfi srfi-1)
  #:use-module (ufw utilities)
  #:export (←
            generate-type-macros
            tabular-macros))

(define (make-frontend-name variant short-type user-pointer?)
  (define (frontend-name)
    (stringify (match variant
                 ((f b) f)
                 (_ variant))))
  (string-append "REG"
                 (if user-pointer? "x_" "_")
                 (string-upcase short-type)
                 (if variant (string-upcase (frontend-name)) "")))

(define (make-backend-name variant user-pointer?)
  (define (backend-name)
    (stringify (match variant
                 ((f b) b)
                 (_ variant))))
  (string-append "MAKE_"
                 (if variant (string-upcase (backend-name)) "")
                 (if variant "_" "")
                 "REGISTER"
                 (if user-pointer? "x" "")))

(define (make-arg-list variant short-type long-type user-pointer? args)
  (let* ((args* (stringify args))
         (long-type* (string-append "REG_TYPE_" (string-upcase long-type)))
         (user-arg (if user-pointer? (! U) '()))
         (frontend-args (append (! I A) args* (! D) user-arg))
         (backend-args (append (! I A)
                               (list long-type* short-type)
                               (! D) args* user-arg)))
    (list user-pointer?
          (string-append (make-frontend-name variant short-type user-pointer?)
                         "(" (comma-list frontend-args) ")")
          (string-append (make-backend-name variant user-pointer?)
                         "(" (comma-list backend-args) ")"))))

(define (make-generator . args)
  (lambda (v s l u)
    (make-arg-list v s l u args)))

(define-syntax-rule (← arg ...)
  (make-generator 'arg ...))

(define (make-type t w)
  (string-append (symbol->string t) (number->string w)))

(define (generate-types ts ws)
  (apply append (map (lambda (t)
                       (map (lambda (w)
                              (cons (make-type (car t) w)
                                    (make-type (cdr t) w)))
                            ws))
                     ts)))

(define (generate-type-macros variants type)
  (define (call-handler h name t)
    (list (h name (car t) (cdr t) #f)
          (h name (car t) (cdr t) #t)))
  (match type
    ((('mnemonics . m) ('widths . w))
     (concatenate
      (map (lambda (type)
             (concatenate
              (map (lambda (variant)
                     (match variant
                       ((name handler)
                        (call-handler handler name type))))
                   variants)))
           (generate-types m w))))))

(define (print-tabular-macro longest modify spec)
  (match spec
    ((user? frontend backend)
     (format #t "#define ~va ~a~%" (modify spec longest) frontend backend))))

(define* (tabular-macros code longest #:key
                         (predicate (λ _ #t))
                         (modify-width (λ (e n) n)))
  (for-each (lambda (spec) (print-tabular-macro longest modify-width spec))
            (filter predicate code)))
