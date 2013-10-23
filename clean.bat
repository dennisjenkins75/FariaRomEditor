@echo off

del /q/s *.ncb *.opt *.obj *.lib *.exe *.plg thumbs.db

for %%P in (FariaRomEditor FariaLib Hacks) do (
  for %%T in (Debug Release Profile DebugUnicode ReleaseUnicode ProfileUnicode) do (
    if exist %%P\%%T\nul (
      rmdir /q/s %%P\%%T
    )
  )
)
