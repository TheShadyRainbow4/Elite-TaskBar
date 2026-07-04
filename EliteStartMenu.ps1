<#
.SYNOPSIS
Elite Start Menu Mirror
#>

#region Initialization & Boilerplate
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

[System.Windows.Forms.Application]::EnableVisualStyles()
[System.Windows.Forms.Application]::SetCompatibleTextRenderingDefault($false)
#endregion

param (
    [int]$MonitorIndex = 0,
    [int]$TaskbarY = 0
)

#region Core Logic
$global:frm_Main = New-Object System.Windows.Forms.Form
$global:frm_Main.Text = "Elite Start Menu"
$global:frm_Main.FormBorderStyle = 'FixedSingle'
$global:frm_Main.ControlBox = $false
$global:frm_Main.ShowInTaskbar = $false
$global:frm_Main.StartPosition = 'Manual'
$global:frm_Main.TopMost = $true
$global:frm_Main.Size = New-Object System.Drawing.Size(380, 500)
$global:frm_Main.BackColor = [System.Drawing.SystemColors]::Control

# Add Client Edge styling
$global:pnl_Main = New-Object System.Windows.Forms.Panel
$global:pnl_Main.BorderStyle = 'Fixed3D'
$global:pnl_Main.Location = New-Object System.Drawing.Point(5, 5)
$global:pnl_Main.Size = New-Object System.Drawing.Size(355, 430)
$global:pnl_Main.BackColor = [System.Drawing.SystemColors]::Window
$global:frm_Main.Controls.Add($global:pnl_Main)

# Left pane (Programs)
$global:lst_Programs = New-Object System.Windows.Forms.ListBox
$global:lst_Programs.Dock = 'Left'
$global:lst_Programs.Width = 200
$global:lst_Programs.BorderStyle = 'None'
$global:lst_Programs.Font = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Regular)
$global:pnl_Main.Controls.Add($global:lst_Programs)

# Right pane (Shortcuts)
$global:lst_Shortcuts = New-Object System.Windows.Forms.ListBox
$global:lst_Shortcuts.Dock = 'Fill'
$global:lst_Shortcuts.BorderStyle = 'None'
$global:lst_Shortcuts.BackColor = [System.Drawing.Color]::FromArgb(255, 230, 240, 255)
$global:lst_Shortcuts.Font = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Regular)
$global:pnl_Main.Controls.Add($global:lst_Shortcuts)

# Populate placeholders
"Accessories", "Administrative Tools", "Startup", "System Tools", "Windows PowerShell", "Command Prompt", "Control Panel", "Run..." | ForEach-Object {
    [void]$global:lst_Programs.Items.Add($_)
}

"Documents", "Pictures", "Music", "Computer", "Network", "Settings", "Search", "Help" | ForEach-Object {
    [void]$global:lst_Shortcuts.Items.Add($_)
}

# The Chin
$global:pnl_Chin = New-Object System.Windows.Forms.Panel
$global:pnl_Chin.Location = New-Object System.Drawing.Point(0, 440)
$global:pnl_Chin.Size = New-Object System.Drawing.Size(380, 60)
$global:pnl_Chin.BackColor = [System.Drawing.Color]::FromArgb(255, 100, 100, 100) # Darker shade for chin
$global:frm_Main.Controls.Add($global:pnl_Chin)

$global:btn_Power = New-Object System.Windows.Forms.Button
$global:btn_Power.Text = "Shut down"
$global:btn_Power.Font = New-Object System.Drawing.Font("Segoe UI", 9, [System.Drawing.FontStyle]::Regular)
$global:btn_Power.Size = New-Object System.Drawing.Size(100, 30)
$global:btn_Power.Location = New-Object System.Drawing.Point(260, 15)
$global:btn_Power.FlatStyle = 'System'
$global:pnl_Chin.Controls.Add($global:btn_Power)

$global:btn_Power.Add_Click({
    # Close menu on action
    $global:frm_Main.Close()
})

$global:frm_Main.Add_Deactivate({
    $global:frm_Main.Close()
})

# Positioning Logic
$global:frm_Main.Add_Load({
    $screens = [System.Windows.Forms.Screen]::AllScreens
    $targetScreen = $screens[0]
    foreach ($screen in $screens) {
        if ($screen.DeviceName -match "DISPLAY$($MonitorIndex + 1)") {
            $targetScreen = $screen
            break
        }
    }
    
    # Bottom Left of the target screen, just above the taskbar
    $x = $targetScreen.Bounds.X
    $y = $targetScreen.Bounds.Y + $targetScreen.Bounds.Height - $global:frm_Main.Height
    
    if ($TaskbarY -gt 0) {
        $y = $TaskbarY - $global:frm_Main.Height
    }
    
    $global:frm_Main.Location = New-Object System.Drawing.Point($x, $y)
})

[void]$global:frm_Main.ShowDialog()
#endregion
