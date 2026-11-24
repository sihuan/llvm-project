; RUN: opt -passes=split-packed-float-pairs -verify-each -S < %s | FileCheck %s

; A 16-byte homogeneous float aggregate is passed/returned by several psABIs
; (e.g. RISC-V LP64D) in a pair of integer registers, so front-ends coerce it
; to [2 x i64].  After mem2reg/SROA the element-wise operations appear as
; bit-packing on the i64 pair, and a loop-carried accumulator becomes an
; integer `or` recurrence instead of a float reduction.  This pass recovers the
; two f32 lanes from that canonical pack so the float dataflow is visible.

; CHECK-LABEL: @packed_accumulate(
; CHECK:       %acc.lo = phi float
; CHECK:       %acc.hi = phi float
; CHECK-NOT:   phi i64
; CHECK-NOT:   shl i64
; CHECK-NOT:   or disjoint i64
define float @packed_accumulate(ptr %p, i32 %n) {
entry:
  br label %loop

loop:
  %i    = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc  = phi i64 [ 0, %entry ], [ %pack, %loop ]
  %lo32 = trunc i64 %acc to i32
  %lo   = bitcast i32 %lo32 to float
  %t    = lshr i64 %acc, 32
  %hi32 = trunc i64 %t to i32
  %hi   = bitcast i32 %hi32 to float
  %addr = getelementptr float, ptr %p, i32 %i
  %v    = load float, ptr %addr
  %nlo  = fadd float %lo, %v
  %nhi  = fadd float %hi, %v
  %nlo32 = bitcast float %nlo to i32
  %nhi32 = bitcast float %nhi to i32
  %zlo  = zext i32 %nlo32 to i64
  %zhi  = zext i32 %nhi32 to i64
  %sh   = shl i64 %zhi, 32
  %pack = or disjoint i64 %zlo, %sh
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  %r = phi float [ %nlo, %loop ]
  ret float %r
}

; The `disjoint` flag is not required: the two zext'd i32 lanes are disjoint by
; construction, so a plain `or` means the same thing.

; CHECK-LABEL: @packed_accumulate_plain_or(
; CHECK:       %acc.lo = phi float
; CHECK:       %acc.hi = phi float
; CHECK-NOT:   phi i64
; CHECK-NOT:   or i64
define float @packed_accumulate_plain_or(ptr %p, i32 %n) {
entry:
  br label %loop

loop:
  %i    = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc  = phi i64 [ 0, %entry ], [ %pack, %loop ]
  %lo32 = trunc i64 %acc to i32
  %lo   = bitcast i32 %lo32 to float
  %t    = lshr i64 %acc, 32
  %hi32 = trunc i64 %t to i32
  %hi   = bitcast i32 %hi32 to float
  %addr = getelementptr float, ptr %p, i32 %i
  %v    = load float, ptr %addr
  %nlo  = fadd float %lo, %v
  %nhi  = fadd float %hi, %v
  %nlo32 = bitcast float %nlo to i32
  %nhi32 = bitcast float %nhi to i32
  %zlo  = zext i32 %nlo32 to i64
  %zhi  = zext i32 %nhi32 to i64
  %sh   = shl i64 %zhi, 32
  %pack = or i64 %zlo, %sh
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  %r = phi float [ %nlo, %loop ]
  ret float %r
}

; A non-matching i64 PHI (bit-sliced lane masks, not a float pack) must be left
; alone.

; CHECK-LABEL: @not_a_float_pack(
; CHECK:       phi i64
define i64 @not_a_float_pack(i64 %a, i64 %b) {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %loop ]
  %x = phi i64 [ %a, %entry ], [ %y, %loop ]
  %lo = and i64 %x, 4294967295
  %hi = and i64 %b, -4294967296
  %y  = or disjoint i64 %lo, %hi
  %i.next = add i64 %i, 1
  %c = icmp ult i64 %i.next, 8
  br i1 %c, label %loop, label %exit

exit:
  %r = phi i64 [ %y, %loop ]
  ret i64 %r
}

; Only constants: there is no real pack, so do not split (this also keeps the
; pass from firing on arbitrary i64 PHIs whose lanes happen to be extracted).

; CHECK-LABEL: @constants_without_pack(
; CHECK:       %x = phi i64
define i64 @constants_without_pack(i64 %a) {
entry:
  br label %loop

loop:
  %x = phi i64 [ 0, %entry ], [ 5, %loop ]
  %lo32 = trunc i64 %x to i32
  %hi = lshr i64 %x, 32
  %hi32 = trunc i64 %hi to i32
  %c = icmp ult i32 %lo32, 8
  br i1 %c, label %loop, label %exit

exit:
  %r = phi i64 [ %x, %loop ]
  ret i64 %r
}

; The lshr has a non-trunc user, so the packed value is used as an integer and
; the PHI must be left alone.

; CHECK-LABEL: @lshr_extra_use(
; CHECK:       %acc = phi i64
define i64 @lshr_extra_use(ptr %p, i32 %n) {
entry:
  br label %loop

loop:
  %i    = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc  = phi i64 [ 0, %entry ], [ %pack, %loop ]
  %lo32 = trunc i64 %acc to i32
  %lo   = bitcast i32 %lo32 to float
  %t    = lshr i64 %acc, 32
  %hi32 = trunc i64 %t to i32
  %hi   = bitcast i32 %hi32 to float
  %addr = getelementptr float, ptr %p, i32 %i
  %v    = load float, ptr %addr
  %nlo  = fadd float %lo, %v
  %nhi  = fadd float %hi, %v
  %nlo32 = bitcast float %nlo to i32
  %nhi32 = bitcast float %nhi to i32
  %zlo  = zext i32 %nlo32 to i64
  %zhi  = zext i32 %nhi32 to i64
  %sh   = shl i64 %zhi, 32
  %pack = or disjoint i64 %zlo, %sh
  %extra = add i64 %t, 1
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  %r = phi i64 [ %extra, %loop ]
  ret i64 %r
}

; A trunc to a narrower type other than i32 is not a lane extract.

; CHECK-LABEL: @trunc_to_i8(
; CHECK:       %acc = phi i64
define i8 @trunc_to_i8(ptr %p, i32 %n) {
entry:
  br label %loop

loop:
  %i    = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc  = phi i64 [ 0, %entry ], [ %pack, %loop ]
  %bad  = trunc i64 %acc to i8
  %lo32 = trunc i64 %acc to i32
  %lo   = bitcast i32 %lo32 to float
  %t    = lshr i64 %acc, 32
  %hi32 = trunc i64 %t to i32
  %hi   = bitcast i32 %hi32 to float
  %addr = getelementptr float, ptr %p, i32 %i
  %v    = load float, ptr %addr
  %nlo  = fadd float %lo, %v
  %nhi  = fadd float %hi, %v
  %nlo32 = bitcast float %nlo to i32
  %nhi32 = bitcast float %nhi to i32
  %zlo  = zext i32 %nlo32 to i64
  %zhi  = zext i32 %nhi32 to i64
  %sh   = shl i64 %zhi, 32
  %pack = or disjoint i64 %zlo, %sh
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  %r = phi i8 [ %bad, %loop ]
  ret i8 %r
}
