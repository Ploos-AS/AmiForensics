/* AmiForensics M2.1d ResidentView ARexx qualification */
options results

out = 'RAM:m2_1d_residentview_result.txt'
if ~open(fh, out, 'W') then exit 20

call writeln(fh, 'GATE=M2_1D_AREXX_RUNTIME')
call writeln(fh, 'PORT=RESIDENTVIEW')

address RESIDENTVIEW
'PING'
ping_rc = RC
ping_result = RESULT
call writeln(fh, 'PING_RC=' || ping_rc)
call writeln(fh, 'PING_RESULT=' || ping_result)

'LIST RESIDENTS'
list_rc = RC
list_result = RESULT
call writeln(fh, 'LIST_RESIDENTS_RC=' || list_rc)
call writeln(fh, 'LIST_RESIDENTS_BEGIN')
call writeln(fh, list_result)
call writeln(fh, 'LIST_RESIDENTS_END')

'QUIT'
quit_rc = RC
quit_result = RESULT
call writeln(fh, 'QUIT_RC=' || quit_rc)
call writeln(fh, 'QUIT_RESULT=' || quit_result)

status = 'FAIL'
if ping_rc = 0 & ping_result = 'PONG' & list_rc = 0 & pos('resident|', list_result) > 0 & quit_rc = 0 & quit_result = 'BYE' then status = 'PASS'
call writeln(fh, 'STATUS=' || status)
call close(fh)

say 'M2.1d result written to' out
if status = 'PASS' then exit 0
exit 5
