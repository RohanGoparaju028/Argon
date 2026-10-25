if exists('b:current_syntax') | finish | endif
syn keyword argonKeyword fn pub priv let const struct class enum test match switch case default if else while for in break continue return import extern cpp unsafe
syn keyword argonType i32 i64 f64 bool string void Ptr Weak Array LinkedList Deque Stack Queue HashMap HashSet
syn keyword argonBoolean true false
syn keyword argonBuiltin print println read_line assert str length weak upgrade is_empty is_expired strong_count
syn match argonNumber '\<\d\+\%(\.\d\+\)\?\%([eE][+-]\?\d\+\)\?\%(i32\|i64\|f64\)\?\>'
syn match argonEscape '\\.' contained
syn region argonString start='"' skip='\\.' end='"' contains=argonEscape
syn match argonComment '//.*$' contains=@Spell
syn region argonComment start='/\*' end='\*/' contains=@Spell
hi def link argonKeyword Keyword
hi def link argonType Type
hi def link argonBoolean Boolean
hi def link argonBuiltin Function
hi def link argonNumber Number
hi def link argonEscape SpecialChar
hi def link argonString String
hi def link argonComment Comment
let b:current_syntax = 'argon'
