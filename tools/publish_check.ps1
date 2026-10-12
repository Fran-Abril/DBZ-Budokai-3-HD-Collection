# dbz3 - tools\publish_check.ps1
# Lint pre-release del arbol versionable (github/). Reimplementacion adaptada de
# `um publish check` (universal-modder) para DBZ3. Detecta, en los ficheros
# RASTREADOS por git (los que se subirian a GitHub):
#   FAIL  ficheros de juego verbatim (extensiones protegidas), secretos (claves
#         API, .env), ficheros de build pesados (exe/dll) rastreados por error.
#   WARN  huellas de decompilador (FUN_xxxx/sub_XXXX/"Decompiled with"), rutas
#         personales absolutas, sin README.
#
# Uso:
#   powershell -ExecutionPolicy Bypass -File tools\publish_check.ps1 [-Repo github]
#
# Codigo de salida: 0 = PASS/WARN, 1 = FAIL. Pensado para correr ANTES de
# tools\make_release.ps1.
param(
    [string]$Repo = "github"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if ([System.IO.Path]::IsPathRooted($Repo)) {
    $repoPath = $Repo
} else {
    $repoPath = Join-Path $root $Repo
}

if (-not (Test-Path -LiteralPath (Join-Path $repoPath ".git"))) {
    Write-Error "publish_check: '$Repo' no es un repo git (falta .git). Ruta: $repoPath"
    exit 2
}

# --- patrones -----------------------------------------------------------------
$gameExt = @(".iso", ".xiso", ".xex", ".afs", ".bin", ".awo", ".azt", ".amb", ".amo", ".amg", ".dds", ".xbr", ".bmp", ".png", ".rgb",
             ".amt", ".rgba", ".npy", ".npz")   # v1.4.0: texturas PS2, previews crudas y datos numpy derivados del juego
$buildExt = @(".exe", ".dll", ".pdb", ".ilk", ".obj", ".tlog", ".zip", ".7z", ".rar")
# excepciones explicitas permitidas (herramientas del pipeline)
$allowBuild = @("tools/xbcompress.exe", "tools/xbdecompress.exe", "tools/xbecompress.exe")
$codeExt = @(".cs", ".c", ".cpp", ".h", ".hpp", ".py", ".lua", ".js", ".ts", ".rs", ".java", ".kt", ".ps1", ".bat", ".sh")
$textExt = $codeExt + @(".json", ".toml", ".ini", ".cfg", ".txt", ".md", ".xml", ".yaml", ".yml", ".env")

$secretRx = @(
    @{ n = "FAL key";              rx = '\b[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}:[0-9a-f]{32}\b' }
    @{ n = "Anthropic key";       rx = 'sk-ant-[A-Za-z0-9_\-]{20,}' }
    @{ n = "OpenAI key";          rx = '\bsk-(?:proj-)?[A-Za-z0-9]{32,}' }
    @{ n = "GitHub token";        rx = '\bgh[pousr]_[A-Za-z0-9]{30,}' }
    @{ n = "AWS key id";          rx = '\bAKIA[0-9A-Z]{16}\b' }
    @{ n = "private key";         rx = '-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----' }
)
$decompRx = @(
    @{ n = "Ghidra auto-name";    rx = '\b(?:FUN|DAT|LAB|PTR)_[0-9a-fA-F]{6,}\b' }
    @{ n = "IDA auto-name";       rx = '\b(?:sub|loc|unk|off|dword|qword)_[0-9A-F]{5,}\b' }
    @{ n = "decompiler header";   rx = '(?i)(Decompiled with|Decompiler:|ILSpy|dnSpy|JetBrains decompiler)' }
)
$userPathRx = '[A-Za-z]:\\Users\\[^\\\s"'']+|/home/[a-z_][a-z0-9_-]*/|/Users/[A-Za-z]+/'

$fails = New-Object System.Collections.Generic.List[string]
$warns = New-Object System.Collections.Generic.List[string]

# --- ficheros rastreados ------------------------------------------------------
$tracked = & git -C $repoPath ls-files
if ($LASTEXITCODE -ne 0) { Write-Error "publish_check: git ls-files fallo"; exit 2 }

foreach ($rel in $tracked) {
    $rel = $rel -replace '\\', '/'
    $ext = [System.IO.Path]::GetExtension($rel).ToLower()
    $full = Join-Path $repoPath ($rel -replace '/', '\')

    if ($gameExt -contains $ext -and $rel -ne 'docs/logo.png') {   # el logo del README es propio, no del juego
        # permitir ficheros de datos legitimos de tools/ (gamecontrollerdb.txt no; .bin si no)
        $fails.Add("game file extension rastreado: $rel")
    }
    if ($buildExt -contains $ext) {
        if ($allowBuild -notcontains $rel) {
            $fails.Add("build artifact rastreado: $rel")
        }
    }
    if ($textExt -contains $ext -or $ext -eq "") {
        if (-not (Test-Path -LiteralPath $full)) { continue }
        $txt = Get-Content -LiteralPath $full -Raw -ErrorAction SilentlyContinue
        if ($null -eq $txt) { continue }
        foreach ($s in $secretRx) { if ($txt -match $s.rx) { $fails.Add("$($s.n) en $rel") } }
        if ($codeExt -contains $ext -and $rel -ne "tools/publish_check.ps1") {
            foreach ($d in $decompRx) {
                $m = [regex]::Matches($txt, $d.rx)
                if ($m.Count -gt 0) { $warns.Add("$($d.n) x$($m.Count) en $rel (e.g. $($m[0].Value))") }
            }
        }
        if ($txt -match $userPathRx) { $warns.Add("ruta personal absoluta en $rel") }
    }
}

# --- README -------------------------------------------------------------------
$names = $tracked | ForEach-Object { [System.IO.Path]::GetFileName($_).ToLower() }
if (-not ($names | Where-Object { $_ -like 'readme*' })) {
    $warns.Add("no hay README rastreado en $Repo")
}

# --- informe ------------------------------------------------------------------
foreach ($x in $fails) { Write-Host "FAIL  $x" -ForegroundColor Red }
foreach ($x in $warns) { Write-Host "WARN  $x" -ForegroundColor Yellow }
$status = if ($fails.Count -gt 0) { "FAIL" } elseif ($warns.Count -gt 0) { "WARN" } else { "PASS" }
Write-Host ("$status : {0} ficheros rastreados, {1} fallos, {2} avisos" -f $tracked.Count, $fails.Count, $warns.Count)
if ($fails.Count -gt 0) { exit 1 } else { exit 0 }
