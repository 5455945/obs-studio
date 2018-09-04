; -- xmf_install.iss --
; -- 2017-08-24
; -- zhangfj

#define MyAppName "xmf"
#define MyAppVersion "1.1.25"
#define MyAppPublisher "SHANGHAI VANGEN NETWORK, Inc."
#define MyAppURL "https://www.xmf.com/"
#define MyAppExeName "xmf.exe"
#define MySourceDir "obs-studio"
#define MyAppId "{{A329A680-0BB1-4BF1-A465-3E9C85AE9587}"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}AppVerName="{#MyAppName} {#MyAppVersion}"
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
;DefaultDirName={pf32}\xmf\{#MyAppName}
DefaultDirName={pf}\xmf\{#MyAppName}
DefaultGroupName={#MyAppName}                                                 
Compression=lzma
SolidCompression=yes
;PrivilegesRequired=poweruser
; win7�������װ����������Ҫ�ù���ԱȨ�ް�װ
PrivilegesRequired=admin

; ��װ����;Password="vangen.cn";Encryption=yes

; windows��������С�ж�ػ���ĳ����б��е�ͼ��UninstallDisplayIcon="{uninstallexe}"
OutputDir=.\
OutputBaseFilename={#MyAppName}Install-{#MyAppVersion}SetupIconFile={#MyAppName}.ico

; ���x64λ�жϳ�ʼ�����ã�ֻ����Setup������ArchitecturesInstallIn64BitMode�Is64BitInstallMode�Ż���Ч
ArchitecturesInstallIn64BitMode=x64

;��ע�汾��Ϣ
InfoBeforeFile="xmf_license.txt"
InfoAfterFile="test_readme.txt"
VersionInfoCompany="https://www.xmf.com/"
VersionInfoDescription="{#MyAppName} ֱ����"
VersionInfoVersion={#MyAppVersion}
VersionInfoCopyright="Copyright (C) 2016-2018 {#MyAppName}"


; ��Ҫ��װ���ļ����������ǿ�������
[Files];Source: "{#MySourceDir}\*"; DestDir: {app}\;Check: Is64BitInstallMode; Flags: ignoreversion overwritereadonly createallsubdirs recursesubdirs; 
Source: "{#MySourceDir}\*"; DestDir: {app}\; Flags: ignoreversion overwritereadonly createallsubdirs recursesubdirs; 

;; ����vs2015����ʱ��
;Source: "{#MySourceDir}\vc2015_redist\vc2015_vcredist_x64.exe"; DestDir: "{tmp}"; Check: NeedInstallVC2015RedistX64
;Source: "{#MySourceDir}\vc2015_redist\vc2015_vcredist_x86.exe"; DestDir: "{tmp}"; Check: NeedInstallVC2015RedistX86
; ��Ҫ��vc2015����ʱ�� msvcp140.dll �� vcruntime140.dll �Ķ�Ӧ�汾����binĿ¼
[Icons]
Name: "{commonprograms}\{#MyAppName}"; Filename: "{app}\bin\32bit\{#MyAppName}32.exe"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\bin\32bit\{#MyAppName}32.exe"
Name: "{commonprograms}\{#MyAppName}Uninstall"; Filename: "{uninstallexe}"
;[Types]
;Name: "full"; Description: "��ȫ��װ"; ;Name: "compact"; Description: "��లװ";
;Name: "custom"; Description: "�Զ��尲װ"; Flags: iscustom;
;[Components];Name: "delAppData"; Description: "ɾ����ʷ������Ϣ"; Check: CheckAppDataXmf;

[Tasks]
Name: taskDelAppData; Description: "ɾ����ʷ������Ϣ[{userappdata}\{#MyAppName}]"; GroupDescription: "������Ϣ:"; Check: CheckAppDataXmf; Flags:unchecked;

; ��װǰҪɾ�����ļ�
[InstallDelete]Type: filesandordirs; Name: "{userappdata}\{#MyAppName}"; Tasks: taskDelAppData;

; ע������
[Registry] 
; ֻ��������Ҫ����Ҫ����ע��������Ĭ�����ע�����

; ��װ��ɺ����е�ѡ��
[Run]
; �������ļ�������ָ��Ŀ¼
Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\vc2015_redist\x86\Microsoft.VC140.CRT\*"" ""{app}\bin\32bit\"""; Flags: hidewizard runhidden

Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\windows10_kit_Redist\D3D\x86\*"" ""{app}\bin\32bit\"""; Flags: hidewizard runhidden

Filename: "{cmd}"; Parameters: "/c copy /y ""{app}\windows10_kit_Redist\ucrt\DLLs\x86\*"" ""{app}\bin\32bit\"""; Flags: hidewizard runhidden

; --------------------------------------------------------------------------------
; �����win8.1 ��Ҫ��װ�ܶಹ����
; ���ڻ��� x64 ��ϵͳ�� Windows 8.1 ���³��� (KB2919442)����װ(KB2919355)���Ⱦ�����
; Windows8.1-KB2919442-x64.msu
; https://www.microsoft.com/zh-cn/download/details.aspx?id=42162

; Windows 8.1 ���� x64 ��ϵͳ�ĸ��� (KB2919355),��װvs2015����ʱ���Ⱦ�����
; clearcompressionflag.exe
; Windows8.1-KB2919355-x64.msu
; Windows8.1-KB2932046-x64.msu
; Windows8.1-KB2934018-x64.msu
; Windows8.1-KB2937592-x64.msu
; Windows8.1-KB2938439-x64.msu
; Windows8.1-KB2959977-x64.msu
; 2.��Щ KB ���밴����˳��װ��clearcompressionflag.exe��KB2919355��KB2932046��KB2959977��KB2937592��KB2938439��KB2934018��
; 3.KB2919442 �� Windows 8.1 ���µ��Ⱦ��������ڳ��԰�װ KB2919355 ֮ǰӦ�Ȱ�װ clearcompressionflag.exe
; https://www.microsoft.com/zh-cn/download/details.aspx?id=42335
; --------------------------------------------------------------------------------


;; ���û������汾��vc2015����ʱ�⣬��װ  ;Filename: "{tmp}\vc2015_vcredist_x64.exe"; Parameters: /q; WorkingDir: {tmp}; Check: NeedInstallVC2015RedistX64; Flags: skipifdoesntexist; StatusMsg: "Installing Microsoft Visual C++ 2015 Redistributable X64 Runtime ..."; 
;Filename: "{tmp}\vc2015_vcredist_x86.exe"; Parameters: /q; WorkingDir: {tmp}; Check: NeedInstallVC2015RedistX86; Flags: skipifdoesntexist; StatusMsg: "Installing Microsoft Visual C++ 2015 Redistributable X86 Runtime ..."; 
;Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\vc2015_redist"""; Flags: hidewizard runhidden

; �ڰ�װ�����������ѡ�����е�ѡ��
Filename: "{app}\bin\32bit\{#MyAppName}32.exe"; Description: "{cm:LaunchProgram,{#MyAppName}X86}"; Flags: nowait postinstall skipifsilent;
; Filename: "{app}\data\obs-studio\license\gplv2.txt"; Description: "�����ļ�"; Flags: postinstall skipifsilent shellexec unchecked;

; ----------------------------------------------------------
; ����ע�����  
Filename: "{app}\regServices.bat"; Flags: nowait runhidden;
; ������������ 
Filename: "{app}\startup_edit.bat"; Flags: nowait runhidden; 
; ----------------------------------------------------------

; ��װ��Ҫɾ�����ļ�
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\regServices.bat"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c del /s /f /q ""{app}\startup_edit.bat"""; Flags: hidewizard runhidden
   

; ɾ�������ļ�
Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\vc2015_redist"""; Flags: hidewizard runhidden
Filename: "{cmd}"; Parameters: "/c rd /s /q ""{app}\windows10_kit_Redist"""; Flags: hidewizard runhidden  


; ��װ������ɺ󣬴�ָ������ҳFilename: "https://www.xmf.com/"; Description: "Զ�̰칫"; Flags: shellexec skipifsilent

[UninstallDelete]
; ж��ʱ��ɾ����װ����Ŀ¼�ļ�
Name: {app}; Type: filesandordirs
Type: filesandordirs; Name: "{userappdata}\{#MyAppName}"; Tasks: taskDelAppData;

[UninstallRun]
; ж����ɺ󣬴�ָ��ҳ��
Filename: "https://www.xmf.com/"; Flags: shellexec;

[Messages]
; ��Щ��Ϣ���ܴ�˫���ţ�����˫����Ҳ����ʾ����  
BeveledLabel={#MyAppPublisher}
; ж�ضԻ���˵��  
ConfirmUninstall="�������Ҫ�ӵ�����ж��{#MyAppName}��?%n%n�� [��] ����ȫɾ�� %1 �Լ������������;%n�� [��]������������������ĵ�����."  
; �����ѹ˵��  StatusExtractFiles=��ѹ�������������ļ�����ؿ��ļ�...


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
var hWnd: HWND;
begin
   // �����Ƿ���ڵ�½����߸��¿�������ڹر�
   hWnd := FindWindowByWindowName('�汾����');
   if (hWnd <> 0) then
   begin
       //MsgBox(' find �汾����', mbInformation, MB_OK);
       PostMessage(hWnd, 18, 0, 0);
   end

   hWnd := FindWindowByWindowName('��ʼ����');
   if (hWnd <> 0) then
   begin
       //MsgBox(' find ��ʼ����', mbInformation, MB_OK);
       PostMessage(hWnd, 18, 0, 0);
   end

  // �����ͬ�汾���л�����Ӧ��GUID��ͬ
  if RegValueExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{dab68466-3a7d-41a8-a5cf-415e3ff8ef71}', 'DisplayName') // Microsoft Visual C++ 2015 Redistributable X64 [Win7/8/8.1/10 64λ V14.0.23918.0]
  then
      begin
        vc2015RedistX64 := false;
      end
  else
      begin
        // �������x64ϵͳ������Ҫ��װ
        vc2015RedistX64 := Is64BitInstallMode;
      end;
  if RegValueExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{2e085fd2-a3e4-4b39-8e10-6b8d35f55244}', 'DisplayName') // Microsoft Visual C++ 2015 Redistributable X86 [Win7/8/8.1/10 32λ V14.0.23918.0]
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

var
  xmfpath: String;
  msg: String;// ж��ʱ������ָ���Ƿ�ɾ��%appdata%\xmf
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    xmfpath := ExpandConstant('{userappdata}');
    xmfpath := xmfpath + '\xmf';
    msg := '���Ƿ�Ҫɾ���û�������Ϣ' + xmfpath + '?';
    if MsgBox(msg, mbConfirmation, MB_YESNO) = IDYES then
    begin
      DelTree(xmfpath, True, True, True);
    end;
  end;
end;
// ��װʱ��ָ���Ƿ�ɾ�� %appdata%\xmf
function CheckAppDataXmf(): Boolean;
begin
    xmfpath := ExpandConstant('{userappdata}');
    xmfpath := xmfpath + '\xmf';
    Result := FileOrDirExists(xmfpath)
end;
