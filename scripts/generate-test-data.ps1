param(
    [string]$DataDirectory = (Join-Path $PSScriptRoot "..\build-debug\bin\Data")
)

$ErrorActionPreference = "Stop"
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$data = [System.IO.Path]::GetFullPath($DataDirectory)
$expectedData = [System.IO.Path]::GetFullPath((Join-Path $projectRoot "build-debug\bin\Data"))
if (-not $data.Equals($expectedData, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "For safety, test data can only reset the Debug data directory: $expectedData"
}

$debugExecutable = Join-Path $projectRoot "build-debug\bin\2048_game_qt.exe"
$debugExecutablePath = [System.IO.Path]::GetFullPath($debugExecutable)
Get-CimInstance Win32_Process -Filter "Name = '2048_game_qt.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -eq $debugExecutablePath } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force }

if (Test-Path -LiteralPath $data) {
    Remove-Item -LiteralPath $data -Recurse -Force
}
$usersRoot = Join-Path $data "Users"
$modsRoot = Join-Path $data "DLC"
New-Item -ItemType Directory -Force -Path $usersRoot | Out-Null
New-Item -ItemType Directory -Force -Path $modsRoot | Out-Null

# Keep the script source ASCII-compatible so Windows PowerShell 5.1 cannot
# misread Chinese literals when the .ps1 file has no UTF-8 BOM.
$timedModeName = ([string][char]0x9650) + ([char]0x65F6) + ([char]0x6A21) + ([char]0x5F0F)
$stepModeName = ([string][char]0x8BA1) + ([char]0x6B65) + ([char]0x6A21) + ([char]0x5F0F)
$customModeName = ([string][char]0x81EA) + ([char]0x5B9A) + ([char]0x4E49) +
                  ([char]0x68CB) + ([char]0x76D8) + ([char]0x6A21) + ([char]0x5F0F)

$xorKey = [Text.Encoding]::UTF8.GetBytes("2048GameRankKey")
function Write-Utf8NoBom([string]$Path, [object[]]$Lines) {
    $text = ($Lines -join [Environment]::NewLine) + [Environment]::NewLine
    [IO.File]::WriteAllText($Path, $text, [Text.UTF8Encoding]::new($false))
}
function ConvertTo-XorHex([string]$Text) {
    $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
    $result = [Text.StringBuilder]::new()
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        [void]$result.AppendFormat("{0:X2}", ($bytes[$i] -bxor $xorKey[$i % $xorKey.Length]))
    }
    $result.ToString()
}

function Get-PasswordHash([string]$Username, [string]$Password) {
    [uint64]$h1 = 0
    [uint64]$h2 = 0
    foreach ($byte in [Text.Encoding]::UTF8.GetBytes("${Username}:$Password")) {
        $h1 = (($h1 * 131) + $byte) % 1000000007
        $h2 = (($h2 * 13331) + $byte) % 1000000009
    }
    @([uint32]$h1, [uint32]$h2)
}

$password = "Test1234"
$samples = @(
    @{ Name="test_alice";   Email="alice@example.com";   Bio="High-score strategy player.";       Score=15680; Tile=2048; Steps=721; Seconds=386; Status="normal" },
    @{ Name="test_bob";     Email="bob@example.com";     Bio="Steady and patient player.";         Score=9200;  Tile=1024; Steps=488; Seconds=302; Status="normal" },
    @{ Name="test_cara";    Email="cara@example.com";    Bio="Working toward the 2048 tile.";      Score=5400;  Tile=512;  Steps=351; Seconds=245; Status="normal" },
    @{ Name="test_dylan";   Email="dylan@example.com";   Bio="Prefers short timed games.";         Score=12800; Tile=2048; Steps=610; Seconds=330; Status="normal" },
    @{ Name="test_eve";     Email="eve@example.com";     Bio="Custom-board mode tester.";          Score=7600;  Tile=1024; Steps=430; Seconds=280; Status="normal" },
    @{ Name="test_banned";  Email="banned@example.com";  Bio="Account reserved for ban testing."; Score=11000; Tile=1024; Steps=590; Seconds=360; Status="banned" },
    @{ Name="test_blocked"; Email="blocked@example.com"; Bio="Second banned test account.";       Score=6800;  Tile=512;  Steps=405; Seconds=275; Status="banned" },
    @{ Name="test_retired"; Email=""; Bio=""; Score=8800;  Tile=1024; Steps=530; Seconds=420; Status="deleted" },
    @{ Name="test_deleted"; Email=""; Bio=""; Score=13200; Tile=2048; Steps=650; Seconds=375; Status="deleted" }
)

$userFile = Join-Path $data "user.dat"
$scoreFile = Join-Path $data "scores.dat"
$accountsFile = Join-Path $data "accounts.ini"
$accountUsers = [ordered]@{}
$accountStatus = [ordered]@{}
$nextUid = 1
if (Test-Path -LiteralPath $accountsFile) {
    $section = ""
    foreach ($line in Get-Content -LiteralPath $accountsFile -Encoding UTF8) {
        $trimmed = $line.Trim().TrimStart([char]0xFEFF)
        if ($trimmed -match '^\[(.+)\]$') {
            $section = $Matches[1]
            continue
        }
        $separator = $trimmed.IndexOf('=')
        if ($separator -lt 1) { continue }
        $key = $trimmed.Substring(0, $separator)
        $value = $trimmed.Substring($separator + 1)
        if ($section -eq 'users') { $accountUsers[$key] = $value }
        elseif ($section -eq 'status') { $accountStatus[$key] = $value }
        elseif ($section -eq 'meta' -and $key -eq 'nextUid') {
            $parsedNextUid = 0
            if ([int]::TryParse($value, [ref]$parsedNextUid)) {
                $nextUid = [Math]::Max($nextUid, $parsedNextUid)
            }
        }
    }
}
foreach ($existingUid in $accountUsers.Values) {
    $numericUid = 0
    if ([int]::TryParse([string]$existingUid, [ref]$numericUid)) {
        $nextUid = [Math]::Max($nextUid, $numericUid + 1)
    }
}
$sampleNames = @($samples | ForEach-Object Name)
$existingUsers = if (Test-Path $userFile) { @(Get-Content -LiteralPath $userFile -Encoding UTF8 | ForEach-Object { $_.TrimStart([char]0xFEFF) }) } else { @() }
$existingScores = if (Test-Path $scoreFile) { @(Get-Content -LiteralPath $scoreFile -Encoding UTF8 | ForEach-Object { $_.TrimStart([char]0xFEFF) }) } else { @() }
$existingUsers = @($existingUsers | Where-Object { $sampleNames -notcontains (($_ -split "`t")[0]) })
$existingScores = @($existingScores | Where-Object { $sampleNames -notcontains (($_ -split "`t")[0]) })

$sampleIndex = 0
foreach ($sample in $samples) {
    if ($accountUsers.Contains($sample.Name)) {
        $uid = [string]$accountUsers[$sample.Name]
    } else {
        do {
            $uid = "{0:D6}" -f $nextUid
            $nextUid++
        } while ($accountUsers.Values -contains $uid -or (Test-Path -LiteralPath (Join-Path $usersRoot $uid)))
        $accountUsers[$sample.Name] = $uid
    }
    $isDeleted = $sample.Status -eq 'deleted'
    $accountStatus[$sample.Name] = $sample.Status
    if (-not $isDeleted) {
        $sampleHashes = Get-PasswordHash $sample.Name $password
        $existingUsers += "$($sample.Name)`t$($sampleHashes[0])`t$($sampleHashes[1])"
    }
    $achieved = [DateTimeOffset]::UtcNow.AddMinutes(-($sampleIndex + 1)).ToUnixTimeSeconds()
    $rankPlain = "$($sample.Score)|$($sample.Tile)|$($sample.Steps)|$($sample.Seconds)|classic|$(if ($isDeleted) { 1 } else { 0 })|$achieved"
    $existingScores += "$($sample.Name)`t$(ConvertTo-XorHex $rankPlain)"

    $folder = Join-Path $usersRoot $uid
    if ($isDeleted) {
        $sampleIndex++
        continue
    }
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    $profile = @(
        "[General]",
        "email=$($sample.Email)",
        "biography=$($sample.Bio)",
        "lastUsernameChange=0"
    )
    Write-Utf8NoBom (Join-Path $folder "profile.ini") $profile
    $settings = @("[General]", "autoSaveInterval=50", "", "[dlc]", "timedSeconds=300", "stepLimit=500", "customBoardSize=5", "", "[keys]", "up=87", "down=83", "left=65", "right=68", "undo=90", "save=67", "restart=82", "menu=66")
    Write-Utf8NoBom (Join-Path $folder "settings.ini") $settings
    $history = @()
    for ($index = 0; $index -lt 3; $index++) {
        $score = $sample.Score - ($index * 850)
        $finished = [DateTimeOffset]::UtcNow.AddDays(-($index + 1)).ToUnixTimeSeconds()
        $plain = "$score|$($sample.Tile)|$($sample.Steps + $index * 20)|$($sample.Seconds + $index * 15)|classic|$finished"
        $history += "$($sample.Name)`t$(ConvertTo-XorHex $plain)"
    }
    Write-Utf8NoBom (Join-Path $folder "history.dat") $history
    $sampleIndex++
}
$accountLines = @("[users]")
foreach ($name in $accountUsers.Keys) { $accountLines += "$name=$($accountUsers[$name])" }
$accountLines += ""
$accountLines += "[status]"
foreach ($name in $accountStatus.Keys) { $accountLines += "$name=$($accountStatus[$name])" }
$accountLines += ""
$accountLines += "[meta]"
$accountLines += ("nextUid={0}" -f $nextUid)

Write-Utf8NoBom $userFile $existingUsers
Write-Utf8NoBom $scoreFile $existingScores
Write-Utf8NoBom $accountsFile $accountLines
Write-Utf8NoBom (Join-Path $modsRoot "timed_300.json") @(
    "{",
    '  "type": "mode",',
    '  "id": "timed_300",',
    ('  "name": "{0}",' -f $timedModeName),
    '  "timeLimitSeconds": 300,',
    '  "rankingEnabled": true',
    "}"
)
Write-Utf8NoBom (Join-Path $modsRoot "step_500.json") @(
    "{",
    '  "type": "mode",',
    '  "id": "step_500",',
    ('  "name": "{0}",' -f $stepModeName),
    '  "stepLimit": 500,',
    '  "rankingEnabled": true',
    "}"
)
Write-Utf8NoBom (Join-Path $modsRoot "custom_board.json") @(
    "{",
    '  "type": "mode",',
    '  "id": "custom_board",',
    ('  "name": "{0}",' -f $customModeName),
    '  "boardSize": 0,',
    '  "rankingEnabled": true',
    "}"
)
Write-Host "All previous Debug data was cleared." -ForegroundColor Yellow
Write-Host "Fresh test users created in: $data" -ForegroundColor Green
Write-Host "DLC catalog created: timed_300, step_500, custom_board" -ForegroundColor Cyan
Write-Host "Normal users: test_alice, test_bob, test_cara, test_dylan, test_eve" -ForegroundColor Green
Write-Host "Banned users: test_banned, test_blocked" -ForegroundColor Red
Write-Host "Deleted users: test_retired, test_deleted" -ForegroundColor DarkGray
Write-Host "Password for normal and banned users: $password" -ForegroundColor Yellow
