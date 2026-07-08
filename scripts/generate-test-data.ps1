param(
    [string]$DataDirectory = (Join-Path $PSScriptRoot "..\build-debug\bin\Data")
)

$ErrorActionPreference = "Stop"
$data = [System.IO.Path]::GetFullPath($DataDirectory)
$usersRoot = Join-Path $data "Users"
New-Item -ItemType Directory -Force -Path $usersRoot | Out-Null

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

function Get-PasswordHash([string]$Password) {
    [uint64]$h1 = 0
    [uint64]$h2 = 0
    foreach ($byte in [Text.Encoding]::UTF8.GetBytes("2048Game:$Password")) {
        $h1 = (($h1 * 131) + $byte) % 1000000007
        $h2 = (($h2 * 13331) + $byte) % 1000000009
    }
    @([uint32]$h1, [uint32]$h2)
}

$password = "Test1234"
$samples = @(
    @{ Name="test_alice"; Email="alice@example.com"; Bio="Enjoys high-score strategies."; Score=15680; Tile=2048; Steps=721; Seconds=386; Deleted=0 },
    @{ Name="test_bob";   Email="bob@example.com";   Bio="A steady and patient player."; Score=9200; Tile=1024; Steps=488; Seconds=302; Deleted=0 },
    @{ Name="test_cara";  Email="cara@example.com";  Bio="Working toward the first 2048 tile."; Score=5400; Tile=512; Steps=351; Seconds=245; Deleted=0 },
    @{ Name="test_retired"; Email=""; Bio=""; Score=8800; Tile=1024; Steps=530; Seconds=420; Deleted=1 }
)

$userFile = Join-Path $data "user.dat"
$scoreFile = Join-Path $data "scores.dat"
$sampleNames = @($samples | ForEach-Object Name)
$existingUsers = if (Test-Path $userFile) { @(Get-Content -LiteralPath $userFile -Encoding UTF8 | ForEach-Object { $_.TrimStart([char]0xFEFF) }) } else { @() }
$existingScores = if (Test-Path $scoreFile) { @(Get-Content -LiteralPath $scoreFile -Encoding UTF8 | ForEach-Object { $_.TrimStart([char]0xFEFF) }) } else { @() }
$existingUsers = @($existingUsers | Where-Object { $sampleNames -notcontains (($_ -split "`t")[0]) })
$existingScores = @($existingScores | Where-Object { $sampleNames -notcontains (($_ -split "`t")[0]) })

$hashes = Get-PasswordHash $password
foreach ($sample in $samples) {
    if ($sample.Deleted -eq 0) {
        $existingUsers += "$($sample.Name)`t$($hashes[0])`t$($hashes[1])"
    }
    $rankPlain = "$($sample.Score)|$($sample.Tile)|$($sample.Steps)|$($sample.Seconds)|classic|$($sample.Deleted)"
    $existingScores += "$($sample.Name)`t$(ConvertTo-XorHex $rankPlain)"

    $folder = Join-Path $usersRoot (ConvertTo-XorHex $sample.Name)
    if (Test-Path $folder) { Remove-Item -LiteralPath $folder -Recurse -Force }
    if ($sample.Deleted -eq 1) { continue }
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    $profile = @(
        "[General]",
        "email=$($sample.Email)",
        "biography=$($sample.Bio)",
        "lastUsernameChange=0"
    )
    Write-Utf8NoBom (Join-Path $folder "profile.ini") $profile
    $settings = @("[General]", "autoSaveInterval=50", "", "[keys]", "up=87", "down=83", "left=65", "right=68", "undo=90", "save=67", "restart=82", "menu=66")
    Write-Utf8NoBom (Join-Path $folder "settings.ini") $settings
    $history = @()
    for ($index = 0; $index -lt 3; $index++) {
        $score = $sample.Score - ($index * 850)
        $finished = [DateTimeOffset]::UtcNow.AddDays(-($index + 1)).ToUnixTimeSeconds()
        $plain = "$score|$($sample.Tile)|$($sample.Steps + $index * 20)|$($sample.Seconds + $index * 15)|classic|$finished"
        $history += "$($sample.Name)`t$(ConvertTo-XorHex $plain)"
    }
    Write-Utf8NoBom (Join-Path $folder "history.dat") $history
}

Write-Utf8NoBom $userFile $existingUsers
Write-Utf8NoBom $scoreFile $existingScores
Write-Host "Test users created in: $data" -ForegroundColor Green
Write-Host "Password for active test users: $password" -ForegroundColor Yellow
