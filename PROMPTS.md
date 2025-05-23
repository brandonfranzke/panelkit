# CLAUDE PROMPTS

## INITIAL

## CONTINUING

I'm continuing work on a Raspberry Pi SDL display issue. Please read the attached PROGRESS.md (panelkit_sdl directory) which contains full context, current status, and the specific problem we're debugging.

The immediate task is to get SDL framebuffer video drivers (KMSDRM or fbdev) compiled into our custom SDL build so that graphics appear on the physical
display. We have a working minimal test framework in place.

Based on the progress report, what's the best next step to debug the EGL/cross-compilation issue that's preventing KMSDRM from compiling?
