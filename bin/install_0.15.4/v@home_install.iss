; -- v@home_install.iss --
; -- 2016-08-27
; -- zhangfj

#define MyAppName "v@home"
#define MyAppVersion "0.15.4"
#define MyAppPublisher "SHANGHAI VANGEN NETWORK, Inc."
#define MyAppURL "https://www.yunvm.com/"
#define MyAppExeName "v@home.exe"
#define MySourceDir "obs-studio-0.15.4"
#define MyAppId "{{A329A680-0BB1-4BF1-A465-3E9C85AE9587}"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}AppVerName="{#MyAppName} {#MyAppVersion}"
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\v@home\{#MyAppName}
DefaultGroupName={#MyAppName}                                                 
Compression=lzma
SolidCompression=yes
;PrivilegesRequired=poweruser
; win7下如果安装后启动，需要用管理员权限安装
PrivilegesRequired=admin

; 安装密码;Password="vangen.cn";Encryption=yes

; windows控制面板中【卸载或更改程序】列表中的图标UninstallDisplayIcon="{uninstallexe}"
OutputDir=.\
OutputBaseFilename={#MyAppName}Install-{#MyAppVersion}SetupIconFile={#MyAppName}.ico

; 添加x64位判断初始化设置，只有在Setup段设置ArchitecturesInstallIn64BitMode项，Is64BitInstallMode才会生效
ArchitecturesInstallIn64BitMode=x64

;备注版本信息
InfoBeforeFile="v@home_license.txt"
InfoAfterFile="test_readme.txt"
VersionInfoCompany="https://www.yunvm.com/"
VersionInfoDescription="{#MyAppName} 直播器"
VersionInfoVersion={#MyAppVersion}
VersionInfoCopyright="Copyright (C) 2016-2018 {#MyAppName}"



; 需要安装的文件处理，基本是拷贝动作
[Files]
; x64系统
Source: "{#MySourceDir}\*"; DestDir: {app}\; Check: Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall; 

; x86系统，不安装x64位文件
Source: "{#MySourceDir}\bin\32bit\*"; DestDir: {app}\bin\32bit\; Check: not Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall; 
Source: "{#MySourceDir}\data\*"; DestDir: {app}\data\; Check: not Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall; 
Source: "{#MySourceDir}\obs-plugins\32bit\*"; DestDir: {app}\obs-plugins\32bit\; Check: not Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall; 
Source: "{#MySourceDir}\vc2015_redist\*"; DestDir: {app}\vc2015_redist\; Check: not Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall;
Source: "{#MySourceDir}\windows10_kit_Redist\*"; DestDir: {app}\windows10_kit_Redist\; Check: not Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall;
Source: "{#MySourceDir}\regServices.bat"; DestDir: {app}\; Check: not Is64BitInstallMode; Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall; 
Source: "{#MySourceDir}\startup_edit.bat"; DestDir: {app}\; Check: not Is64BitInstallMode;Flags: replacesameversion createallsubdirs recursesubdirs uninsneveruninstall; 

;; 处理vs2015运行时库
;Source: "{#MySourceDir}\vc2015_redist\vc2015_vcredist_x64.exe"; DestDir: "{tmp}"; Check: NeedInstallVC2015RedistX64
;Source: "{#MySourceDir}\vc2015_redist\vc2015_vcredist_x86.exe"; DestDir: "{tmp}"; Check: NeedInstallVC2015RedistX86
; 需要把vc2015运行时的 msvcp140.dll 和 vcruntime140.dll 的对应版本放入bin目录
[Icons]
Name: "{commonprograms}\{#MyAppName}X64"; Filename: "{app}\bin\64bit\{#MyAppName}64.exe"; Check: Is64BitInstallMode;
Name: "{commondesktop}\{#MyAppName}X64"; Filename: "{app}\bin\64bit\{#MyAppName}64.exe";  Check: Is64BitInstallMode;
Name: "{commonprograms}\{#MyAppName}"; Filename: "{app}\bin\32bit\{#MyAppName}32.exe"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\bin\32bit\{#MyAppName}32.exe"
Name: "{commonprograms}\{#MyAppName}Uninstall"; Filename: "{uninstallexe}"

; 安装前要删除的文件
[InstallDelete]
;Type: files; Name: "{src}\a.exe"
;Type: filesandordirs; Name: "{src}\xxxx"

; 注册表操作
[Registry] 
; 只有特殊需要，才要设置注册表，程序会默认添加注册表项


; 安装完成后，运行的选项
[Run]
; 把依赖文件拷贝到指定目录
Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\vc2015_redist\x86\Microsoft.VC140.CRT\*"" ""{app}\bin\32bit\"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\vc2015_redist\x64\Microsoft.VC140.CRT\*"" ""{app}\bin\64bit\"""; Check: Is64BitInstallMode; Flags: hidewizard runhidden

Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\windows10_kit_Redist\D3D\x86\*"" ""{app}\bin\32bit\"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\windows10_kit_Redist\D3D\x64\*"" ""{app}\bin\64bit\"""; Check: Is64BitInstallMode; Flags: hidewizard runhidden

Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\windows10_kit_Redist\ucrt\DLLs\x86\*"" ""{app}\bin\32bit\"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\windows10_kit_Redist\ucrt\DLLs\x64\*"" ""{app}\bin\64bit\"""; Check: Is64BitInstallMode; Flags: hidewizard runhidden

; --------------------------------------------------------------------------------
; 如果是win8.1 需要安装很多补丁包
; 用于基于 x64 的系统的 Windows 8.1 更新程序 (KB2919442)，安装(KB2919355)的先决条件
; Windows8.1-KB2919442-x64.msu
; https://www.microsoft.com/zh-cn/download/details.aspx?id=42162

; Windows 8.1 基于 x64 的系统的更新 (KB2919355),安装vs2015运行时的先决条件
; clearcompressionflag.exe
; Windows8.1-KB2919355-x64.msu
; Windows8.1-KB2932046-x64.msu
; Windows8.1-KB2934018-x64.msu
; Windows8.1-KB2937592-x64.msu
; Windows8.1-KB2938439-x64.msu
; Windows8.1-KB2959977-x64.msu
; 2.这些 KB 必须按以下顺序安装：clearcompressionflag.exe、KB2919355、KB2932046、KB2959977、KB2937592、KB2938439、KB2934018。
; 3.KB2919442 是 Windows 8.1 更新的先决条件，在尝试安装 KB2919355 之前应先安装 clearcompressionflag.exe
; https://www.microsoft.com/zh-cn/download/details.aspx?id=42335
; --------------------------------------------------------------------------------


;; 如果没有这个版本的vc2015运行时库，安装  ;Filename: "{tmp}\vc2015_vcredist_x64.exe"; Parameters: /q; WorkingDir: {tmp}; Check: NeedInstallVC2015RedistX64; Flags: skipifdoesntexist; StatusMsg: "Installing Microsoft Visual C++ 2015 Redistributable X64 Runtime ..."; 
;Filename: "{tmp}\vc2015_vcredist_x86.exe"; Parameters: /q; WorkingDir: {tmp}; Check: NeedInstallVC2015RedistX86; Flags: skipifdoesntexist; StatusMsg: "Installing Microsoft Visual C++ 2015 Redistributable X86 Runtime ..."; 
;Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\vc2015_redist"""; Flags: hidewizard runhidden

; 在安装界面最后，设置选择运行的选项
Filename: "{app}\bin\32bit\{#MyAppName}32.exe"; Description: "{cm:LaunchProgram,{#MyAppName}X86}"; Flags: nowait postinstall skipifsilent;
Filename: "{app}\bin\64bit\{#MyAppName}64.exe"; Description: "{cm:LaunchProgram,{#MyAppName}X64}"; Check: Is64BitInstallMode; Flags: nowait postinstall skipifsilent unchecked; 
; Filename: "{app}\data\obs-studio\license\gplv2.txt"; Description: "自述文件"; Flags: postinstall skipifsilent shellexec unchecked;


; ----------------------------------------------------------
; 运行注册程序  
Filename: "{app}\regServices.bat"; Flags: nowait runhidden;
; 运行启动程序 
Filename: "{app}\startup_edit.bat"; Flags: nowait runhidden; 
; ----------------------------------------------------------

; 安装后，要删除的文件
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\regServices.bat"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\startup_edit.bat"""; Flags: hidewizard runhidden

; 如果是32的系统，需要把x64位文件删除
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\win-capture\get-graphics-offsets64.exe"""; Check: not Is64BitInstallMode; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\win-capture\graphics-hook64.dll"""; Check: not Is64BitInstallMode; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\win-capture\inject-helper64.exe"""; Check: not Is64BitInstallMode; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\obs-ffmpeg\ffmpeg-mux64.exe"""; Check: not Is64BitInstallMode; Flags: hidewizard runhidden


; 不知道为啥，在32为系统上面，会多出两个x64的空文件夹，删除掉
Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\bin\64bit"""; Check: not Is64BitInstallMode; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\obs-plugins\64bit"""; Check: not Is64BitInstallMode; Flags: hidewizard runhidden    

; 删除依赖文件
Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\vc2015_redist"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\windows10_kit_Redist"""; Flags: hidewizard runhidden  

; 不知道为何，这几个文件存在，会导致错误发生
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\win-capture\get-graphics-offsets64.exe"""; Check: Is64BitInstallMode; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\win-capture\graphics-hook64.dll"""; Check: Is64BitInstallMode; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\data\obs-plugins\win-capture\inject-helper64.exe"""; Check: Is64BitInstallMode; Flags: hidewizard runhidden


; 安装程序完成后，打开指定的网页
Filename: "https://www.yunvm.com/"; Description: "打开百度"; Flags: shellexec skipifsilent

[UninstallDelete]
; 卸载是，删除安装程序目录文件
Name: {app}; Type: filesandordirs 

[UninstallRun]
; 卸载完成后，打开指定页面
Filename: "http://www.vangen.cn/"; Flags: nowait shellexec;

[Messages]
; 有些消息不能带双引号，否则，双引号也会显示出来  
BeveledLabel={#MyAppPublisher}
; 卸载对话框说明  
ConfirmUninstall="您真的想要从电脑中卸载{#MyAppName}吗?%n%n按 [是] 则完全删除 %1 以及它的所有组件;%n按 [否]则让软件继续留在您的电脑上."  
; 定义解压说明  StatusExtractFiles=解压并复制主程序文件及相关库文件...


[Code]
var
  vc2015RedistX64: Boolean;
  vc2015RedistX86: Boolean;

function NeedInstallVC2015RedistX64(): Boolean;
begin
  Result := vc2015RedistX64;
end;
function NeedInstallVC2015RedistX86(): Boolean;
begin
  Result := vc2015RedistX86;
end;

function InitializeSetup(): Boolean;
begin
  // 这里，不同版本运行环境对应的GUID不同
  if RegValueExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{dab68466-3a7d-41a8-a5cf-415e3ff8ef71}', 'DisplayName') // Microsoft Visual C++ 2015 Redistributable X64 [Win7/8/8.1/10 64位 V14.0.23918.0]
  then
      begin
        vc2015RedistX64 := false;
      end
  else
      begin
        // 如果不是x64系统，不需要安装
        vc2015RedistX64 := Is64BitInstallMode;
      end;
  if RegValueExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{2e085fd2-a3e4-4b39-8e10-6b8d35f55244}', 'DisplayName') // Microsoft Visual C++ 2015 Redistributable X86 [Win7/8/8.1/10 32位 V14.0.23918.0]
  then
      begin
        vc2015RedistX86 := false;
      end
  else
      begin
        vc2015RedistX86 := true;
      end;

  result := true;
end;
