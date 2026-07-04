<#
.SYNOPSIS
Elite Taskbar Settings (WinForms)
#>
$ErrorActionPreference = 'Stop'

$mutexName = "Global\EliteSettingsMutex_19a796c0"
$createdNew = $false
$mutex = New-Object System.Threading.Mutex($true, $mutexName, [ref]$createdNew)
if (-not $createdNew) {
    [System.Windows.Forms.MessageBox]::Show("Elite Taskbar Settings is already open.", "Elite Taskbar", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Information)
    exit
}

#region Initialization & Boilerplate
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

[System.Windows.Forms.Application]::EnableVisualStyles()
[System.Windows.Forms.Application]::SetCompatibleTextRenderingDefault($false)

# P/Invoke for broadcasting changes
$code = @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, UIntPtr wParam, IntPtr lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
    
    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint RegisterWindowMessage(string lpString);
}
"@
Add-Type -TypeDefinition $code

$WM_SETTINGCHANGE = 0x001A
$HWND_BROADCAST = [IntPtr]0xFFFF
#endregion

#region Core Variables
$global:regPathElite = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
$global:regPathNative = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced"

# Ensure registry keys exist
if (!(Test-Path $global:regPathElite)) { New-Item -Path $global:regPathElite -Force | Out-Null }

$baseFont = New-Object System.Drawing.Font("Segoe UI", 9, [System.Drawing.FontStyle]::Regular)
$boldFont = New-Object System.Drawing.Font("Segoe UI", 9, [System.Drawing.FontStyle]::Semibold)
$titleFont = New-Object System.Drawing.Font("Segoe UI", 12, [System.Drawing.FontStyle]::Semibold)

$colorChin = [System.Drawing.Color]::FromArgb(255, 240, 240, 240)
#endregion

#region Form Construction
$frm_Main = New-Object System.Windows.Forms.Form
$frm_Main.Text = "Taskbar and Start Menu Properties"
$frm_Main.ClientSize = New-Object System.Drawing.Size(550, 450)
$frm_Main.FormBorderStyle = 'FixedDialog'
$frm_Main.MaximizeBox = $false
$frm_Main.StartPosition = 'CenterScreen'
$frm_Main.Font = $baseFont

# Banner
$pnl_Banner = New-Object System.Windows.Forms.Panel
$pnl_Banner.Size = New-Object System.Drawing.Size(550, 60)
$pnl_Banner.Location = New-Object System.Drawing.Point(0, 0)
$pnl_Banner.BackColor = [System.Drawing.Color]::White

$lbl_TitleBanner = New-Object System.Windows.Forms.Label
$lbl_TitleBanner.Text = "Elite Taskbar Configuration"
$lbl_TitleBanner.Font = $titleFont
$lbl_TitleBanner.Location = New-Object System.Drawing.Point(60, 20)
$lbl_TitleBanner.AutoSize = $true
$pnl_Banner.Controls.Add($lbl_TitleBanner)

# 3D Inset Area
$pnl_Inset = New-Object System.Windows.Forms.Panel
$pnl_Inset.Size = New-Object System.Drawing.Size(530, 320)
$pnl_Inset.Location = New-Object System.Drawing.Point(10, 70)
$pnl_Inset.BorderStyle = 'Fixed3D'

# Tabs
$tab_Main = New-Object System.Windows.Forms.TabControl
$tab_Main.Dock = 'Fill'
$pnl_Inset.Controls.Add($tab_Main)

# Tab: Taskbar
$tp_Taskbar = New-Object System.Windows.Forms.TabPage
$tp_Taskbar.Text = "Taskbar"
$tp_Taskbar.UseVisualStyleBackColor = $true
$tab_Main.Controls.Add($tp_Taskbar)

$grp_Mode = New-Object System.Windows.Forms.GroupBox
$grp_Mode.Text = "Taskbar Mode"
$grp_Mode.Location = New-Object System.Drawing.Point(15, 15)
$grp_Mode.Size = New-Object System.Drawing.Size(490, 60)
$tp_Taskbar.Controls.Add($grp_Mode)

$rdo_ModeIndep = New-Object System.Windows.Forms.RadioButton
$rdo_ModeIndep.Text = "Independent"
$rdo_ModeIndep.Location = New-Object System.Drawing.Point(15, 25)
$rdo_ModeIndep.AutoSize = $true
$grp_Mode.Controls.Add($rdo_ModeIndep)

$rdo_ModeReplace = New-Object System.Windows.Forms.RadioButton
$rdo_ModeReplace.Text = "Replace"
$rdo_ModeReplace.Location = New-Object System.Drawing.Point(130, 25)
$rdo_ModeReplace.AutoSize = $true
$grp_Mode.Controls.Add($rdo_ModeReplace)

$rdo_ModeSecond = New-Object System.Windows.Forms.RadioButton
$rdo_ModeSecond.Text = "Secondary Only"
$rdo_ModeSecond.Location = New-Object System.Drawing.Point(230, 25)
$rdo_ModeSecond.AutoSize = $true
$grp_Mode.Controls.Add($rdo_ModeSecond)

$grp_App = New-Object System.Windows.Forms.GroupBox
$grp_App.Text = "Taskbar Appearance"
$grp_App.Location = New-Object System.Drawing.Point(15, 90)
$grp_App.Size = New-Object System.Drawing.Size(490, 80)
$tp_Taskbar.Controls.Add($grp_App)

$lbl_BtnWidth = New-Object System.Windows.Forms.Label
$lbl_BtnWidth.Text = "Button Width:"
$lbl_BtnWidth.Location = New-Object System.Drawing.Point(15, 25)
$lbl_BtnWidth.AutoSize = $true
$grp_App.Controls.Add($lbl_BtnWidth)

$rdo_WidthAuto = New-Object System.Windows.Forms.RadioButton
$rdo_WidthAuto.Text = "Auto"
$rdo_WidthAuto.Location = New-Object System.Drawing.Point(100, 23)
$rdo_WidthAuto.AutoSize = $true
$grp_App.Controls.Add($rdo_WidthAuto)

$rdo_WidthFixed = New-Object System.Windows.Forms.RadioButton
$rdo_WidthFixed.Text = "Fixed"
$rdo_WidthFixed.Location = New-Object System.Drawing.Point(160, 23)
$rdo_WidthFixed.AutoSize = $true
$grp_App.Controls.Add($rdo_WidthFixed)

$rdo_WidthIcons = New-Object System.Windows.Forms.RadioButton
$rdo_WidthIcons.Text = "Icons Only"
$rdo_WidthIcons.Location = New-Object System.Drawing.Point(220, 23)
$rdo_WidthIcons.AutoSize = $true
$grp_App.Controls.Add($rdo_WidthIcons)

$chk_Previews = New-Object System.Windows.Forms.CheckBox
$chk_Previews.Text = "Show window previews"
$chk_Previews.Location = New-Object System.Drawing.Point(15, 50)
$chk_Previews.AutoSize = $true
$grp_App.Controls.Add($chk_Previews)

# Tab: Multi-Monitor
$tp_MultiMon = New-Object System.Windows.Forms.TabPage
$tp_MultiMon.Text = "Multi-Monitor Components"
$tp_MultiMon.UseVisualStyleBackColor = $true
$tab_Main.Controls.Add($tp_MultiMon)

$pnl_MonScroll = New-Object System.Windows.Forms.Panel
$pnl_MonScroll.AutoScroll = $true
$pnl_MonScroll.Dock = 'Fill'
$pnl_MonScroll.BackColor = [System.Drawing.Color]::Transparent
$tp_MultiMon.Controls.Add($pnl_MonScroll)

# Load Monitors dynamically
$monitors = [System.Windows.Forms.Screen]::AllScreens
$y = 10
$global:monControls = @()
$global:orbFiles = Get-ChildItem -Path "$PSScriptRoot\..\Resources\StartOrb" -Filter "*.png" | Select-Object -ExpandProperty FullName

for ($i=0; $i -lt $monitors.Count; $i++) {
    $grp = New-Object System.Windows.Forms.GroupBox
    $grp.Text = "Monitor $i ($($monitors[$i].Bounds.Width)x$($monitors[$i].Bounds.Height))"
    $grp.Location = New-Object System.Drawing.Point(15, $y)
    $grp.Size = New-Object System.Drawing.Size(480, 180)
    $pnl_MonScroll.Controls.Add($grp)

    $chk_Tray = New-Object System.Windows.Forms.CheckBox
    $chk_Tray.Text = "System Tray"
    $chk_Tray.Location = New-Object System.Drawing.Point(15, 25)
    $chk_Tray.AutoSize = $true
    $grp.Controls.Add($chk_Tray)

    $chk_Clock = New-Object System.Windows.Forms.CheckBox
    $chk_Clock.Text = "Clock"
    $chk_Clock.Location = New-Object System.Drawing.Point(120, 25)
    $chk_Clock.AutoSize = $true
    $grp.Controls.Add($chk_Clock)

    $chk_TaskBtns = New-Object System.Windows.Forms.CheckBox
    $chk_TaskBtns.Text = "Task Buttons"
    $chk_TaskBtns.Location = New-Object System.Drawing.Point(200, 25)
    $chk_TaskBtns.AutoSize = $true
    $grp.Controls.Add($chk_TaskBtns)

    $lbl_SmMode = New-Object System.Windows.Forms.Label
    $lbl_SmMode.Text = "Start Menu Mode:"
    $lbl_SmMode.Location = New-Object System.Drawing.Point(15, 60)
    $lbl_SmMode.AutoSize = $true
    $grp.Controls.Add($lbl_SmMode)

    $cmb_SmMode = New-Object System.Windows.Forms.ComboBox
    $cmb_SmMode.DropDownStyle = 'DropDownList'
    $cmb_SmMode.Items.AddRange(@("0 - Native / OpenShell Mirror", "1 - Custom Elite Menu", "2 - Exe Launcher"))
    $cmb_SmMode.Location = New-Object System.Drawing.Point(140, 57)
    $cmb_SmMode.Size = New-Object System.Drawing.Size(200, 25)
    $grp.Controls.Add($cmb_SmMode)

    $lbl_SmTrig = New-Object System.Windows.Forms.Label
    $lbl_SmTrig.Text = "Start Menu Trigger:"
    $lbl_SmTrig.Location = New-Object System.Drawing.Point(15, 95)
    $lbl_SmTrig.AutoSize = $true
    $grp.Controls.Add($lbl_SmTrig)

    $cmb_SmTrig = New-Object System.Windows.Forms.ComboBox
    $cmb_SmTrig.DropDownStyle = 'DropDownList'
    $cmb_SmTrig.Items.AddRange(@("0 - Default", "1 - Alternate"))
    $cmb_SmTrig.Location = New-Object System.Drawing.Point(140, 92)
    $cmb_SmTrig.Size = New-Object System.Drawing.Size(200, 25)
    $grp.Controls.Add($cmb_SmTrig)

    $lbl_SmOrb = New-Object System.Windows.Forms.Label
    $lbl_SmOrb.Text = "Start Orb Theme:"
    $lbl_SmOrb.Location = New-Object System.Drawing.Point(15, 130)
    $lbl_SmOrb.AutoSize = $true
    $grp.Controls.Add($lbl_SmOrb)

    $cmb_SmOrb = New-Object System.Windows.Forms.ComboBox
    $cmb_SmOrb.DropDownStyle = 'DropDownList'
    foreach ($orb in $global:orbFiles) {
        $cmb_SmOrb.Items.Add([System.IO.Path]::GetFileNameWithoutExtension($orb)) | Out-Null
    }
    if ($cmb_SmOrb.Items.Count -eq 0) { $cmb_SmOrb.Items.Add("Default") | Out-Null }
    $cmb_SmOrb.Location = New-Object System.Drawing.Point(140, 127)
    $cmb_SmOrb.Size = New-Object System.Drawing.Size(200, 25)
    $grp.Controls.Add($cmb_SmOrb)

    $pic_Orb = New-Object System.Windows.Forms.PictureBox
    $pic_Orb.Location = New-Object System.Drawing.Point(360, 50)
    $pic_Orb.Size = New-Object System.Drawing.Size(54, 100)
    $pic_Orb.SizeMode = 'Zoom'
    $grp.Controls.Add($pic_Orb)

    $cmb_SmOrb.Add_SelectedIndexChanged({
        param($sender, $e)
        if ($sender.SelectedIndex -ge 0 -and $sender.SelectedIndex -lt $global:orbFiles.Count) {
            $sender.Tag.Image = [System.Drawing.Image]::FromFile($global:orbFiles[$sender.SelectedIndex])
        }
    })
    $cmb_SmOrb.Tag = $pic_Orb

    $global:monControls += @{
        Index = $i;
        ChkTray = $chk_Tray;
        ChkClock = $chk_Clock;
        ChkTaskBtns = $chk_TaskBtns;
        CmbMode = $cmb_SmMode;
        CmbTrig = $cmb_SmTrig;
        CmbOrb = $cmb_SmOrb;
    }

    $y += 195
}

# Chin
$pnl_Chin = New-Object System.Windows.Forms.Panel
$pnl_Chin.Size = New-Object System.Drawing.Size(550, 50)
$pnl_Chin.Location = New-Object System.Drawing.Point(0, 400)
$pnl_Chin.BackColor = $colorChin

$btn_Apply = New-Object System.Windows.Forms.Button
$btn_Apply.Text = "Apply"
$btn_Apply.Location = New-Object System.Drawing.Point(450, 12)
$pnl_Chin.Controls.Add($btn_Apply)

$btn_Okay = New-Object System.Windows.Forms.Button
$btn_Okay.Text = "Okay"
$btn_Okay.Location = New-Object System.Drawing.Point(365, 12)
$pnl_Chin.Controls.Add($btn_Okay)

$btn_Cancel = New-Object System.Windows.Forms.Button
$btn_Cancel.Text = "Cancel"
$btn_Cancel.Location = New-Object System.Drawing.Point(280, 12)
$pnl_Chin.Controls.Add($btn_Cancel)

$frm_Main.Controls.Add($pnl_Banner)
$frm_Main.Controls.Add($pnl_Inset)
$frm_Main.Controls.Add($pnl_Chin)

#endregion

#region Logic
function Load-Settings {
    try {
        $val = (Get-ItemProperty -Path $global:regPathElite -Name "TaskbarMode" -ErrorAction SilentlyContinue).TaskbarMode
        if ($val -eq 0) { $rdo_ModeIndep.Checked = $true }
        elseif ($val -eq 1) { $rdo_ModeReplace.Checked = $true }
        else { $rdo_ModeSecond.Checked = $true }

        $val = (Get-ItemProperty -Path $global:regPathElite -Name "TaskbarButtonWidth" -ErrorAction SilentlyContinue).TaskbarButtonWidth
        if ($val -eq 0) { $rdo_WidthAuto.Checked = $true }
        elseif ($val -eq 1) { $rdo_WidthFixed.Checked = $true }
        else { $rdo_WidthIcons.Checked = $true }

        $val = (Get-ItemProperty -Path $global:regPathElite -Name "TaskbarPreviews" -ErrorAction SilentlyContinue).TaskbarPreviews
        if ($val -eq 0) { $chk_Previews.Checked = $false } else { $chk_Previews.Checked = $true }

        foreach ($mc in $global:monControls) {
            $idx = $mc.Index
            $mc.ChkTray.Checked = ((Get-ItemProperty -Path $global:regPathElite -Name "ShowTray_$idx" -ErrorAction SilentlyContinue)."ShowTray_$idx" -ne 0)
            $mc.ChkClock.Checked = ((Get-ItemProperty -Path $global:regPathElite -Name "ShowClock_$idx" -ErrorAction SilentlyContinue)."ShowClock_$idx" -ne 0)
            $mc.ChkTaskBtns.Checked = ((Get-ItemProperty -Path $global:regPathElite -Name "ShowTaskBtns_$idx" -ErrorAction SilentlyContinue)."ShowTaskBtns_$idx" -ne 0)
            
            $val = (Get-ItemProperty -Path $global:regPathElite -Name "StartMenuMode_$idx" -ErrorAction SilentlyContinue)."StartMenuMode_$idx"
            if ($val -ge 0 -and $val -lt $mc.CmbMode.Items.Count) { $mc.CmbMode.SelectedIndex = $val } else { $mc.CmbMode.SelectedIndex = 0 }

            $val = (Get-ItemProperty -Path $global:regPathElite -Name "StartMenuTrigger_$idx" -ErrorAction SilentlyContinue)."StartMenuTrigger_$idx"
            if ($val -ge 0 -and $val -lt $mc.CmbTrig.Items.Count) { $mc.CmbTrig.SelectedIndex = $val } else { $mc.CmbTrig.SelectedIndex = 0 }

            $val = (Get-ItemProperty -Path $global:regPathElite -Name "StartMenuOrb_$idx" -ErrorAction SilentlyContinue)."StartMenuOrb_$idx"
            if ($val -ge 0 -and $val -lt $mc.CmbOrb.Items.Count) { $mc.CmbOrb.SelectedIndex = $val } else { $mc.CmbOrb.SelectedIndex = 0 }
        }
    } catch {}
}

function Save-Settings {
    try {
        $mode = if ($rdo_ModeIndep.Checked) { 0 } elseif ($rdo_ModeReplace.Checked) { 1 } else { 2 }
        Set-ItemProperty -Path $global:regPathElite -Name "TaskbarMode" -Value $mode -Type DWord

        $width = if ($rdo_WidthAuto.Checked) { 0 } elseif ($rdo_WidthFixed.Checked) { 1 } else { 2 }
        Set-ItemProperty -Path $global:regPathElite -Name "TaskbarButtonWidth" -Value $width -Type DWord

        $prev = if ($chk_Previews.Checked) { 1 } else { 0 }
        Set-ItemProperty -Path $global:regPathElite -Name "TaskbarPreviews" -Value $prev -Type DWord

        foreach ($mc in $global:monControls) {
            $idx = $mc.Index
            Set-ItemProperty -Path $global:regPathElite -Name "ShowTray_$idx" -Value $(if ($mc.ChkTray.Checked) {1} else {0}) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "ShowClock_$idx" -Value $(if ($mc.ChkClock.Checked) {1} else {0}) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "ShowTaskBtns_$idx" -Value $(if ($mc.ChkTaskBtns.Checked) {1} else {0}) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "StartMenuMode_$idx" -Value $($mc.CmbMode.SelectedIndex) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "StartMenuTrigger_$idx" -Value $($mc.CmbTrig.SelectedIndex) -Type DWord
            Set-ItemProperty -Path $global:regPathElite -Name "StartMenuOrb_$idx" -Value $($mc.CmbOrb.SelectedIndex) -Type DWord
        }

        # Broadcast change
        $result = [UIntPtr]::Zero
        [Win32]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, [IntPtr]::Zero, 2, 1000, [ref]$result) | Out-Null
        
        $ELITE_UPDATE = [Win32]::RegisterWindowMessage("EliteTaskbarSettingsUpdate")
        [Win32]::SendMessageTimeout($HWND_BROADCAST, $ELITE_UPDATE, [UIntPtr]::Zero, [IntPtr]::Zero, 2, 1000, [ref]$result) | Out-Null

    } catch {}
}

$btn_Apply.Add_Click({ Save-Settings })
$btn_Okay.Add_Click({ Save-Settings; $frm_Main.Close() })
$btn_Cancel.Add_Click({ $frm_Main.Close() })

Load-Settings
$frm_Main.ShowDialog() | Out-Null
#endregion
