// Compiler settings shared by the SimpleDelphiBLE units.

{$IFDEF FPC}
  // Free Pascal compiles the Delphi dialect, with string as a UTF-16 UnicodeString like Delphi 2009 and later.
  {$MODE DELPHIUNICODE}
{$ENDIF}

// SimpleCBLE has the macOS and Linux advanced functions only in builds for those platforms.
{$IF (DEFINED(DARWIN) or DEFINED(MACOS)) and not DEFINED(IOS)}
  {$DEFINE SIMPLEDELPHIBLE_MACOS}
{$ENDIF}
{$IF DEFINED(LINUX) and not DEFINED(ANDROID)}
  {$DEFINE SIMPLEDELPHIBLE_LINUX}
{$ENDIF}
