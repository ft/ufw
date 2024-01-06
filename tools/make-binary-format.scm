;; Copyright (c) 2023-2024 ufw workers, All rights reserved.
;;
;; Terms for redistribution and use can be found in LICENCE.

;; Generate binary-format API.
;;
;; This code is pretty repetitive, and typing is out by hand is error prone.

(use-modules (ice-9 format)
             (ice-9 match)
             (rnrs bytevectors)
             (srfi srfi-1)
             (srfi srfi-19)
             (srfi srfi-42))

(define (c-inclusion-guard-start)
  (format #t "#ifndef INC_UFW_BINARY_FORMAT_H
#define INC_UFW_BINARY_FORMAT_H

"))

(define (c-inclusion-guard-end)
  (format #t "
#endif /* INC_UFW_BINARY_FORMAT_H */
"))

(define (c-include-system sym)
  (format #t "#include <~a.h>~%" sym))

(define (c-include-ufw sym)
  (format #t "#include <ufw/~a.h>~%" sym))

(define (make-copyright)
  (format #t "/*
 * Copyright (c) 2019-~d ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 *
 * This file is generated by ‘tools/make-binary-format’.
 */
"
          (date-year (current-date))))

(define (make-file-doc)
  (format #t
          "/**
 * @file binary-format.h
 * @brief Binary format conversion API
 *
 * This module implements a number of functions that revolve around reading and
 * writing value to/from raw memory. That is three octet-orders (endianness):
 * Native, big and little; where native endianness is one of the other two.
 *
 * The API follows a naming scheme:
 *
 *     bf_[OPERATION]_[TYPEMNEMONIC][WIDTH][ORDER](...)
 *
 * Where OPERATION is either `ref` or `set`; and
 *
 * Where TYPEMNEMONIC is either `u` or `s` or `f` for unsigned integers, signed
 * integers and floating point values respectively; and
 *
 * Where OPERATION is either `ref` or `set`; and
 *
 * Where WIDTH is a width designation in bits. Integer operations support 16,
 * 32 and 64 bits; floating point values support 32 and 64 bits; and
 *
 * Where ORDER is either `n` or `b` or `l` for native, big or little endianness
 * respectively.
 *
 * To put a 32-bit floating point value into memory, in big-endian octet order:
 *
 * @code
 *     uint8_t memory[128];
 *     // ...
 *     bf_set_f32b(memory, 123.f);
 * @endcode
 *
 * Integer behaviour is pretty portable these days. Two's complement is used
 * universally in all modern architectures. There are things like IBM main
 * frames that use BCD for example, and there will be other examples, that
 * could be brought up here.
 *
 * Floating point numbers are less portable, but IEEE754 is pretty common.
 * This module implements all accesses on the architecture's representation.
 *
 * The module works with big- and little-endian archtectures and using bytes
 * of either eight or sixteen bit size.
 *
 * If your system does not use two's complement, signed-integer semantics will
 * be off. And similarly if your system does not use IEEE754 floating point
 * encoding, those semantics will be off.
 */
"))

(define (make-c++-warning:type-punning)
  (format #t "#ifdef __cplusplus
#ifndef CXX_ALLOW_TYPE_PUNNING
#warning \"binary-format uses type punning, which is undefined behaviour in C++!\"
#warning \"Your toolchain may allow it as an extension, but be advised!\"
#warning \"To disable this warning, define the CXX_ALLOW_TYPE_PUNNING macro.\"
#endif /* CXX_ALLOW_TYPE_PUNNING */
#endif /* __cplusplus */
"))

(define (make-ensure-octet-order)
  (format #t "#if !(defined(SYSTEM_ENDIANNESS_BIG)) && !(defined(SYSTEM_ENDIANNESS_LITTLE))
#error \"System octet-order is not indicated! Cannot use binary-format.h for that reason!\"
#endif /* !(defined(SYSTEM_ENDIANNESS_*)) */
"))

(define (make-ensure-byte-size)
  (format #t "#if !(UFW_BITS_PER_BYTE == 8 || UFW_BITS_PER_BYTE == 16)
#error \"System byte-size is unsupported! Cannot use binary-format.h for that reason!\"
#endif /* Unsupported Byte Size */
"))

(define (make-conversion-unions)
  (display "
union bf_convert16 {
    uint16_t u16;
    int16_t s16;
};

union bf_convert32 {
    uint32_t u32;
    int32_t s32;
    float f32;
};

union bf_convert64 {
    uint64_t u64;
    int64_t s64;
    double f64;
};
"))

(define *bits-per-nibble* 4)
(define *bits-per-octet* (* 2 *bits-per-nibble*))
(define *all-bits-octet* (1- (ash 1 *bits-per-octet*)))
(define *powers-of-two* (list-ec (: i 4 7) (ash 1 i)))
(define *word-sizes* (list-ec (: i
                                 (first *powers-of-two*)
                                 (1+ (last *powers-of-two*))
                                 *bits-per-octet*)
                              i))

(define *indent* (make-string 4 #\space))
(define *return-expr* (string-concatenate (list *indent* "return ( ")))
(define *or-expr* (string-concatenate
                   (list (make-string 1 #\newline) (make-string (- (string-length *return-expr*) 2)
                                         #\space)
                         "| ")))
(define *end-expr* ");")

(define (bits->digits n)
  "Calculate number of hex digits for a given number of bits.

Example: (bits->digits 16) → 4"
  (inexact->exact (ceiling (/ n *bits-per-nibble*))))

(define (max-integer-width lst)
  "Calculate number of digits for the largest integer in a list.

Examples:
  (max-integer-width '(1 2 9))   → 1
  (max-integer-width '(1 2 10))  → 2
  (max-integer-width '(1 2 100)) → 3"
  (inexact->exact (ceiling (log10 (+ 1 (apply max lst))))))

(define (make-mask-and-shifter sym bits shift-align literal-suffix)
  "Return a function that generates a sub-expression in a swap expression.

A sub-expression looks like this ((value & mask) OPERATOR shift).

The tabular alignment is handed in as arguments to the generator function."
  (lambda (mask op shift)
    (format #f "((~a & 0x~v,'0x~a) ~a ~vdu)"
            sym (bits->digits bits) mask literal-suffix op shift-align shift)))

(define (make-swap-down-shifts n)
  "Generate a list of down shifts required for a swap expression."
  (list-ec (: i (- n *bits-per-octet*) 0 (* -2 *bits-per-octet*)) i))

(define (make-swap-masks n)
  "Generate a list of bits-mask for a given word-width"
  (list-ec (:do ((i (* *all-bits-octet*
                       (ash 1 (- n *bits-per-octet*)))))
                (> i 0)
                ((ash i (* -1 *bits-per-octet*))))
           i))

(define (make-swap-args n)
  "Generate a list of arguments for mask-and-shifter function for word width n"
  (let* ((down* (make-swap-down-shifts n))
         (down (map (lambda (x) (list '>> x)) down*))
         (up (map (lambda (x) (list '<< x)) (reverse down*))))
    (map (lambda (mask x) (cons mask x))
         (make-swap-masks n)
         (concatenate (list down up)))))

(define (maybe-builtin-swap n)
  (format #t "#if defined(HAVE_COMPILER_BUILTIN_BSWAP~d) && defined(UFW_USE_BUILTIN_SWAP)
    return __builtin_bswap~d(value);
#else
" n n))

(define (end-builtin-swap n)
  (format #t "#endif /* HAVE_COMPILER_BUILTIN_BSWAP~d */~%" n))

(define (bvref n bv endianness)
  ((case n
     ((16) bytevector-u16-ref)
     ((32) bytevector-u32-ref)
     ((64) bytevector-u64-ref)
     (else (throw 'unsupported-swap-word-width n)))
   bv 0 endianness))

(define (make-swap-apidoc n)
  (let ((example #vu8(#x12 #x34 #x56 #x78 #x90 #xab #xcd #xef))
        (digits (bits->digits n)))
    (format #t "/**
 * Byte-swap ~d bit value
 *
 * Turn `0x~v,'0x` into `0x~v,'0x`.
 *
 * @param  value   The value to transform
 *
 * @return The byte-swapped value.
 * @sideeffects None.
 */~%"
            n
            digits (bvref n example 'big)
            digits (bvref n example 'little))))

(define (make-swap-octets n literal-suffix)
  "Generate an octet swapping function for word width n.

The literal-suffix argument is used for the bitmask literals in the mask and
shift expressions."
  (newline)
  (make-swap-apidoc n)
  (format #t "static inline uint~d_t~%bf_swap~d(const uint~d_t value)~%{~%"
          n n n)
  (maybe-builtin-swap n)
  (display *return-expr*)
  (let ((shift-align (max-integer-width (make-swap-down-shifts n))))
    (display
     (string-join (map (lambda (args)
                         (apply (make-mask-and-shifter 'value
                                                       n
                                                       shift-align
                                                       literal-suffix)
                                args))
                       (make-swap-args n))
                  *or-expr*)))
  (display *end-expr*)
  (newline)
  (end-builtin-swap n)
  (format #t "}~%"))

(define (make-word-copy-expression dst src)
  (lambda (idx)
    (format #f "~a[~du] = ~a[~du];" dst idx src idx)))

(define (indent)
  (display "    "))

(define steps '(8 16))

(define (make-reference-prototype width type endianness)
  (format #t "static inline uint~d_t~%bf_ref_~a~d~a(const void *ptr)~%{~%"
          width type width endianness))

(define (make-reference/apidoc type order)
  (format #t "
/**
 * Read ~a datum from buffer in ~a octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the return
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 *
 * @return A ~a datum read from memory pointed to by ptr in
 *         ~a octet order.
 * @sideeffects None.
 */
"
          type order type order))
(define (make-set!/apidoc type order)
  (format #t "
/**
 * Store ~a datum into a buffer in ~a octet order
 *
 * The buffer pointed to must obviously be able to carry a datum of the integer
 * value's size.
 *
 * @param  ptr    Pointer to memory from which to read data.
 * @param  value  Value to store into memory
 *
 * @return Pointer to memory after the newly stored value.
 * @sideeffects None.
 */
"
          type order))

(define (make-reference-native width literal-suffix)
  (make-reference/apidoc (format #f "uint~d_t" width) "native")
  (make-reference-prototype width 'u 'n)
  (indent)
  (format #t "uint~d_t buffer = 0~a;~%" width literal-suffix)
  (indent)
  (format #t "const unsigned char *src = ptr;~%")
  (indent)
  (format #t "unsigned char *dst = (unsigned char*)&buffer;~%")

  (for-each (lambda (step)
              (format #t "#if UFW_BITS_PER_BYTE == ~d~%" step)
              (let loop ((n width)
                         (i 0)
                         (generate (make-word-copy-expression 'dst 'src)))
                (cond ((> n 0)
                       (indent)
                       (display (generate i))
                       (newline)
                       (loop (- n step) (1+ i) generate))
                      (else #f)))
              (format #t "#endif /* UFW_BITS_PER_BYTE == ~d */~%" step))
            steps)

  (indent)
  (format #t "return buffer;~%")
  (format #t "}~%"))

(define (make-set!-prototype width type endianness)
  (format #t "static inline void*~%bf_set_~a~d~a(void *ptr, const uint~d_t value)~%{~%"
          type width endianness width))

(define (make-set!-native width)
  (make-set!/apidoc (format #f "uint~d_t" width) "native")
  (make-set!-prototype width 'u 'n)
  (indent)
  (format #t "const unsigned char *src = (const unsigned char*)&value;~%")
  (indent)
  (format #t "unsigned char *dst = ptr;~%")

  (for-each (lambda (step)
              (format #t "#if UFW_BITS_PER_BYTE == ~d~%" step)
              (let loop ((n width)
                         (i 0)
                         (generate (make-word-copy-expression 'dst 'src)))
                (cond ((> n 0)
                       (indent)
                       (display (generate i))
                       (newline)
                       (loop (- n step) (1+ i) generate))
                      (else #f)))
              (format #t "#endif /* UFW_BITS_PER_BYTE == ~d */~%" step))
            steps)

  (indent)
  (format #t "return dst + sizeof(value);~%")
  (format #t "}~%"))

(define (make-ref-and-swap width)
  (format #t "return bf_swap~d(bf_ref_u~dn(ptr));~%" width width))

(define (make-ref-return width with-swap?)
  (if with-swap?
      (make-ref-and-swap width)
      (format #t "return bf_ref_u~dn(ptr);~%" width)))

(define (make-reference width big?)
  (make-reference/apidoc (format #f "uint~d_t" width)
                         (if big? "big endian" "little endian"))
  (make-reference-prototype width 'u (if big? 'b 'l))
  (format #t "#if defined(SYSTEM_ENDIANNESS_BIG)~%")
  (indent)
  (make-ref-return width (not big?))
  (format #t "#elif defined(SYSTEM_ENDIANNESS_LITTLE)~%")
  (indent)
  (make-ref-return width big?)
  (format #t "#else~%")
  (indent)
  (format #t "/* Top of file names sure this can't happen. */~%")
  (format #t "#endif /* SYSTEM_ENDIANNESS_* */~%")
  (format #t "}~%"))

(define (make-swap-and-set width)
  (format #t "return bf_set_u~dn(ptr, bf_swap~d(value));~%" width width))

(define (make-set!-return width with-swap?)
  (if with-swap?
      (make-swap-and-set width)
      (format #t "return bf_set_u~dn(ptr, value);~%" width)))

(define (make-set! width big?)
  (make-set!/apidoc (format #f "uint~d_t" width)
                    (if big? "big endian" "little endian"))
  (make-set!-prototype width 'u (if big? 'b 'l))
  (format #t "#if defined(SYSTEM_ENDIANNESS_BIG)~%")
  (indent)
  (make-set!-return width (not big?))
  (format #t "#elif defined(SYSTEM_ENDIANNESS_LITTLE)~%")
  (indent)
  (make-set!-return width big?)
  (format #t "#else~%")
  (indent)
  (format #t "/* Top of file names sure this can't happen. */~%")
  (format #t "#endif /* SYSTEM_ENDIANNESS_* */~%")
  (format #t "}~%"))

(define (make-int-type-namer m)
  (lambda (w)
    (format #f "~a~d_t" m w)))

(define (make-float-type-namer m)
  (lambda (_)
    (symbol->string m)))

(define other-types `(((mnemonics (s . ,(make-int-type-namer 'int)))
                       (widths . ,*powers-of-two*))
                      ((mnemonics (f . ,(make-float-type-namer 'float)))
                       (widths 32))
                      ((mnemonics (f . ,(make-float-type-namer 'double)))
                       (widths 64))))

(define (for-typespec fnc typespec)
  (for-each
   (lambda (type)
     (let ((mns (assq-ref type 'mnemonics))
           (widths (assq-ref type 'widths))
           (endians '(n l b)))
       (for-each
        (lambda (endian)
          (for-each
           (lambda (mnemonic)
             (let ((short (car mnemonic))
                   (type-namer (cdr mnemonic)))
               (for-each
                (lambda (width)
                  (fnc endian short (type-namer width) width))
                widths)))
           mns))
        endians)
       ))
   typespec))

(define (make-other-reference typespec)
  (for-typespec (lambda (endian short type width)
                  (make-reference/apidoc type (if (eq? endian 'b)
                                                  "big endian"
                                                  "little endian"))
                  (format #t "static inline ~a
bf_ref_~a~d~a(const void *ptr)
{
    const union bf_convert~d data = { .u~d = bf_ref_u~d~a(ptr) };
    return data.~a~d;
}
"
                          type short width endian
                          width width width endian
                          short width))
                typespec))

(define (make-other-set! typespec)
  (for-typespec (lambda (endian short type width)
                  (make-set!/apidoc type (if (eq? endian 'b)
                                             "big endian"
                                             "little endian"))
                  (format #t "static inline void*
bf_set_~a~d~a(void *ptr, const ~a value)
{
    const union bf_convert~d data = { .~a~d = value };
    return bf_set_u~d~a(ptr, data.u~d);
}
"
                          short width endian type
                          width short width
                          width endian width))
                typespec))

(make-copyright)
(newline)
(make-file-doc)
(newline)
(c-inclusion-guard-start)
(c-include-system 'stdint)
(newline)
(c-include-ufw 'bit-operations)
(c-include-ufw 'toolchain)
(newline)
(make-c++-warning:type-punning)
(newline)
(make-ensure-octet-order)
(newline)
(make-ensure-byte-size)
(make-conversion-unions)
(for-each make-swap-octets *powers-of-two* '(u ul ull))
(for-each make-reference-native *powers-of-two* '(u ul ull))
(for-each (lambda (w) (make-reference w #t)) *powers-of-two*)
(for-each (lambda (w) (make-reference w #f)) *powers-of-two*)
(make-other-reference other-types)
(for-each make-set!-native *powers-of-two*)
(for-each (lambda (w) (make-set! w #t)) *powers-of-two*)
(for-each (lambda (w) (make-set! w #f)) *powers-of-two*)
(make-other-set! other-types)
(c-inclusion-guard-end)
