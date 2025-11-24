param(
    [string] $asm = "tests\branch.asm",
    [int] $maxCycles = 10000
)

function RunAndParse($modeFlag){
    $cmd = ".\conpa.exe -i $asm $modeFlag -c $maxCycles"
    $out = & cmd /c $cmd 2>&1
    $txt = $out -join "`n"
    $cycles = 0
    $insts = 0
    if ($txt -match "Cycles\s*=\s*(\d+)") { $cycles = [int]$matches[1] }
    if ($txt -match "Instructions\s*=\s*(\d+)") { $insts = [int]$matches[1] }
    if ($txt -match "Completed instructions\s*=\s*(\d+)") { $insts = [int]$matches[1] }
    return @{ mode=$modeFlag; text=$txt; cycles=$cycles; insts=$insts }
}

$s_single = RunAndParse "-s"
$s_pipe   = RunAndParse "-p"

# Compute metrics
$c1 = [double]$s_single.cycles; $i1 = [double]$s_single.insts
$c2 = [double]$s_pipe.cycles;   $i2 = [double]$s_pipe.insts
$cpi1 = if ($i1 -gt 0) { $c1 / $i1 } else { [double]::NaN }
$cpi2 = if ($i2 -gt 0) { $c2 / $i2 } else { [double]::NaN }
$speedup = if ($c2 -gt 0) { $c1 / $c2 } else { [double]::NaN }

Write-Host "Results for $asm"
Write-Host "Single-cycle: cycles=$($s_single.cycles) insts=$($s_single.insts) CPI=$([math]::Round($cpi1,3))"
Write-Host "Pipelined:    cycles=$($s_pipe.cycles) insts=$($s_pipe.insts) CPI=$([math]::Round($cpi2,3))"
Write-Host "Speedup (cycles_single / cycles_pipe) = $([math]::Round($speedup,3))"