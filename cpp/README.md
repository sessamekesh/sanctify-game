# C++ Sources Root

- `/cmake`: CMake scripts with functions, macros, and external dependency wrangling.
- `/extern`: External dependencies that cannot be wrangled with CMake's `FetchContent` module
- `/libs`: Indigo libraries (copied around from project to project because they're great)
- `/tools`: Source code for offline build tools used to generate asset files, etc.
- `/sanctify-game-common`: Common code shared between the sanctify game server and client (main game logic)
- `/sanctify-game-server`: Source code for sanctify game server executable
- `/sanctify-game-client`: Source code for Sanctify game client library (including entry point for native executable)