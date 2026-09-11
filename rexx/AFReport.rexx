/* AmiForensics M6.3 ARexx automation
 *
 * Usage:
 *   RX AFReport.rexx before.kv after.kv compare.kv report.kv
 *
 * Compare and Report must be available in the command path.
 */

parse arg before after compareOut reportOut

if before = '' | after = '' | compareOut = '' | reportOut = '' then do
    say 'Usage: RX AFReport.rexx before.kv after.kv compare.kv report.kv'
    exit 10
end

address command

'Compare --kv "'before'" "'after'" >"'compareOut'"'
compareRC = rc
if compareRC ~= 0 & compareRC ~= 5 then do
    say 'AFReport: Compare failed with RC' compareRC
    exit compareRC
end

'Report --kv "'before'" "'after'" "'compareOut'" >"'reportOut'"'
reportRC = rc
if reportRC ~= 0 & reportRC ~= 5 then do
    say 'AFReport: Report failed with RC' reportRC
    exit reportRC
end

if compareRC = 5 | reportRC = 5 then do
    say 'AFReport: completed with bounded-input truncation'
    exit 5
end

say 'AFReport: PASS'
exit 0
