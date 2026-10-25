if exists('b:did_argon_ftplugin') | finish | endif
let b:did_argon_ftplugin = 1
setlocal commentstring=//\ %s
setlocal comments=s1:/*,mb:*,ex:*/,://
setlocal suffixesadd=.ar,.fe
setlocal expandtab shiftwidth=4 softtabstop=4
command! -buffer ArgonRun call argon#run(0)
command! -buffer ArgonRunProject call argon#run(1)
command! -buffer ArgonCheck call argon#check(0)
command! -buffer ArgonCheckProject call argon#check(1)
command! -buffer ArgonBuild call argon#build(1)
command! -buffer ArgonDebug call argon#debug(0)
command! -buffer ArgonDebugProject call argon#debug(1)
let b:undo_ftplugin = 'setlocal commentstring< comments< suffixesadd< expandtab< shiftwidth< softtabstop< | silent! delcommand ArgonRun | silent! delcommand ArgonRunProject | silent! delcommand ArgonCheck | silent! delcommand ArgonCheckProject | silent! delcommand ArgonBuild | silent! delcommand ArgonDebug | silent! delcommand ArgonDebugProject | unlet! b:did_argon_ftplugin'
