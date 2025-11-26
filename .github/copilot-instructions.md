# Global Copilot Instructions for relmer

## Code Formatting - CRITICAL RULES

### **NEVER** Delete Blank Lines
- **NEVER** delete blank lines between file-level constructs (functions, classes, structs)
- **NEVER** delete blank lines between different groups (e.g., C++ includes vs C includes)
- **NEVER** delete blank lines between variable declaration blocks
- Preserve all existing vertical spacing in code

### Project-Specific Vertical Spacing

#### Top-Level Constructs (File Scope)
- **EXACTLY 5 blank lines** between all top-level file constructs:
  - Between preprocessor directives (#include, #define, etc.) and first function
  - Between include blocks and namespace declarations
  - Between namespace and struct/class definitions
  - Between structs/classes and global variables
  - Between global variables and first function
  - Between all function definitions
  - **After the last function in the file**
- **NEVER** add more than 5 blank lines
- **NEVER** delete blank lines if it would result in fewer than 5

#### Function/Block Internal Spacing
- **EXACTLY 3 blank lines** between variable definitions at the top of a function/block and the first real statement
- **2 blank lines** between semantically different variable groups within functions
- **1 blank line** for standard code separation within functions

#### Correct Spacing Example:
```cpp
#include "pch.h"

#include "Header.h"




namespace ns = std::something;




struct MyStruct
{
    int value;
};




static int g_globalVar = 0;




void Function1()
{
    Type var1;
    Type var2;


    Type var3 = value;  // Different semantic group

    // Code section
    DoSomething();
}




void Function2()
{
    // ...
}
```

#### Critical Process for Preserving Spacing:
1. **Before ANY edit**: Run `git show HEAD:filename` to verify exact original spacing
2. **Count blank lines explicitly** - NEVER assume the count
3. **When adding new top-level code**: Add 5 blank lines before and after
4. **After editing**: Run `git diff` to verify no blank lines were accidentally removed
5. **If using `write_to_file`**: Explicitly recreate ALL original blank lines
6. **Always verify** these rules have been adhered to when making changes
7. **Always verify** after making changes that spacing is correct

### Special Code Patterns

#### SetColor Function
- **NEVER** alter the escape sequence passed to `format()` in `SetColor`
- The escape sequence **MUST** always begin with `\x1b[` (ESC + `[`)
- This is critical for ANSI color code functionality

### **NEVER** Break Column Alignment
- **NEVER** break existing column alignment in variable declarations
- **NEVER** break alignment of:
  - Type names
  - Pointer/reference symbols (`*`, `&`) - these form their own column
  - Variable names
  - Assignment operators (`=`)
  - Initialization values
- **ALWAYS** preserve exact column positions when replacing lines
- When modifying a line, ensure replacement maintains same indentation as original
- Pointer/reference symbols spacing:
  - **One space before and after** when NOT part of a column-aligned block
  - **Multiple spaces allowed** when needed to maintain column alignment in a contiguous block
  - Columnar alignment is required whenever there are contiguous lines of declarations/definitions (can include blank lines within the block)

### Column Alignment for Assignment Statements
- **Column alignment applies to assignment statements**, not just variable declarations
- When multiple assignment statements appear consecutively, align the `=` operators
- This creates visual clarity and makes code easier to scan
- **For nested struct members**: Align ALL assignments in a block to the longest member name, even if mixing nested and non-nested members
- **Shader code in string literals**: Column alignment rules apply to HLSL/GLSL shader code within raw string literals - align struct member declarations with `:` (semantic) operators

**Correct** (column-aligned assignments):
```cpp
// Store dimensions for viewport and render target sizing
m_renderWidth  = width;
m_renderHeight = height;
```

**Wrong** (misaligned assignments):
```cpp
// Store dimensions for viewport and render target sizing
m_renderWidth = width;
m_renderHeight = height;
```

**Example with longer variable names**:
```cpp
m_instanceBufferCapacity = INITIAL_CAPACITY;
m_renderWidth            = width;
m_renderHeight           = height;
m_bloomIntensity         = 2.5f;
```

**Correct handling of nested struct members** (all aligned to longest name):
```cpp
swapChainDesc.BufferCount                        = 2;
swapChainDesc.BufferDesc.Width                   = width;
swapChainDesc.BufferDesc.Height                  = height;
swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
swapChainDesc.SampleDesc.Count                   = 1;
```

**Wrong** (inconsistent alignment between nested/non-nested):
```cpp
swapChainDesc.BufferCount                       = 2;
swapChainDesc.BufferDesc.Width                  = width;
swapChainDesc.BufferDesc.RefreshRate.Numerator  = 60;
swapChainDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // WRONG - different column
swapChainDesc.SampleDesc.Count     = 1;  // WRONG - different column
```

**Shader code alignment** (HLSL struct members with semantics):
```cpp
// Correct - colon operators aligned
struct VSInput
{
    float3 position   : POSITION;
    float2 uvMin      : TEXCOORD0;
    float2 uvMax      : TEXCOORD1;
    float4 color      : COLOR;
    float  brightness : BRIGHTNESS;
    float  scale      : SCALE;
    uint   instanceID : SV_InstanceID;
};

// Wrong - unaligned semantics
struct VSInput
{
    float3 position : POSITION;
    float2 uvMin : TEXCOORD0;
    float2 uvMax : TEXCOORD1;
    float4 color : COLOR;
    float brightness : BRIGHTNESS;
    float scale : SCALE;
    uint instanceID : SV_InstanceID;
};
```

### Indentation Rules
- **ALWAYS** preserve exact indentation when replacing code
- **NEVER** start code at column 1 unless original was at column 1
- Count spaces carefully - if original had 12 spaces, replacement must have 12 spaces
- Use spaces for indentation (match existing code style)

### Example of CORRECT editing:
```cpp
// Original:
  std::wcerr << L"Error: " << msg << L"\n";

// CORRECT replacement (preserves 12-space indentation):
            g_pConsole->Printf (CConfig::Error, L"Error: %s\n", msg);

// WRONG replacement (broken indentation):
g_pConsole->Printf (CConfig::Error, L"Error: %s\n", msg);
```

---

## File Modification Rules

### Files Copied From Other Projects
- **NEVER** modify files that are stated to be "copied from another project"
- **NEVER** add `using` declarations to headers copied from other projects
- If those files need types, add `using namespace std;` to `pch.h` instead
- Examples: `Config.h`, `Config.cpp`, `Console.h`, `Console.cpp`, `Color.h`, `AnsiCodes.h`

### Scope of Changes
- **ONLY** modify the files explicitly requested
- If a change requires modifying other files, **ASK FIRST**
- When told to modify file X, do not make "helpful" changes to files Y or Z

---

## Code Changes - Best Practices

### When Replacing Code
1. Read the original line(s) carefully
2. Note the exact indentation level (count spaces)
3. Note any column alignment with surrounding lines
4. Apply changes while preserving formatting
5. Double-check indentation before submitting

### When Showing Code Changes
- **NEVER** show full file contents unless explicitly asked
- Use minimal, surgical edits with `// ...existing code...` comments
- Show only the lines being changed plus minimal context

### Before Applying Changes
- Verify you understand which files should/shouldn't be modified
- Check if files are from other projects (read-only)
- Confirm you're preserving all formatting rules above

---

## C++ Specific Guidelines

### Smart Pointers
- Prefer references over pointers when possible (no ownership, just access)
- Use smart pointers for ownership:
  - Prefer `unique_ptr` for exclusive ownership
  - Use `shared_ptr` when shared ownership is needed
- Consider raw pointers only for non-owning references when references won't work (observers)
- For signal handlers: use smart pointers for global objects that need cleanup on `std::exit()`

### Modern C++ Features
- **ALWAYS** prefer to use the highest-available version of C++ (C++26 (VC++ supported preview features), C++23, C++20, etc.)
- Use `using namespace std;` in `pch.h` for convenience (project preference)
- Prefer `<filesystem>` over old file APIs
- Use `std::atomic` for thread-safe flags
- Use structured bindings where appropriate: `for (const auto& [key, value] : map)`
- Use `std::ranges` and `std::views` where appropriate
- Use `std::format` for string formatting, not `sprintf`
- Use `constexpr` for compile-time constants
- Use `enum class` over plain `enum`
- Use `nullptr` not `NULL`

### Error Handling with EHM (Error Handling Macros)

#### Core EHM Principles
When EHM is present in a codebase (included via `pch.h`), use it consistently for error handling:

**Basic Rules:**
- All functions returning `HRESULT` must declare `HRESULT hr = S_OK;` as first variable
- Include `Error:` label for cleanup in functions returning `HRESULT`
- **NEVER** omit the `Error:` label even if currently unused
- **NEVER** embed function calls within EHM macro parameters
- Variables must be declared at the top of their scope (before any `goto Error` can occur)
- Use `{ }` blocks to create new scopes when local variables are needed mid-function

**Correct Pattern:**
```cpp
HRESULT MyFunction()
{
    HRESULT hr = S_OK;
    
    // Declare ALL variables at top of scope
    ComPtr<ID3D11Device> device;
    DWORD flags = 0;
    
    hr = CreateDeviceFunction (&device);
    CHRA (hr);  // Check result, NOT CreateDeviceFunction() directly
    
    hr = device->SomeMethod (flags);
    CHRA (hr);
    
Error:
    return hr;
}
```

**Wrong Pattern:**
```cpp
// WRONG - don't do this:
CHRA (CreateDeviceFunction (&device));  // Function call embedded in macro!

// WRONG - variable declared after potential goto:
CHRA (hr);
ComPtr<ID3D11Device> device;  // Can't jump past this declaration!
```

#### EHM Macro Variants

**Assert vs Non-Assert:**
- Use `-A` suffix macros (`CHRA`, `CBRA`, `CWRA`) when calling **external APIs**:
  - Windows APIs, DirectX, COM methods, third-party libraries
  - These trigger debug break at the **actual point of failure**
- Use non-`-A` macros (`CHR`, `CBR`, `CWR`) when calling **internal project functions**:
  - Propagates error without additional assert
  - Prevents redundant debug breaks as error bubbles up call stack

**Example:**
```cpp
// External API - use Assert version:
hr = m_device->CreateVertexShader (blob, size, nullptr, &shader);
CHRA (hr);  // Assert on API failure

// Internal function - use non-Assert version:
hr = CreateDevice();
CHR (hr);   // Propagate error without assert
```

**Logging Variants:**
- Use `-L` suffix (`CHRL`, `CHRLA`) **only when providing useful diagnostic information**
- Don't use `-L` for redundant messages like "Function X failed"
- File and line number are logged automatically, so only add `-L` for:
  - Variable values that help diagnose issues
  - Contextual information not obvious from file:line
  - State information that aids debugging

**Example:**
```cpp
// No -L needed - file:line tells us what failed:
hr = m_device->CreateVertexShader (...);
CHRA (hr);

// -L adds useful context:
hr = m_device->CreateTexture2D (&desc, nullptr, &texture);
CHRLA (hr, "Texture dimensions: %dx%d", width, height);

// -L provides context about why:
CBRLA (width > 0 && height > 0, E_INVALIDARG, "Invalid dimensions (0x0), skipping bloom");
```

#### Common EHM Macros
- `CHR(hr)` - Check HRESULT, goto Error if failed
- `CHRA(hr)` - Check HRESULT with assert
- `CHRL(hr, msg, ...)` - Check HRESULT with logging
- `CHRLA(hr, msg, ...)` - Check HRESULT with assert and logging
- `CBR(condition)` - Check boolean, goto Error if false
- `CBRA(condition)` - Check boolean with assert
- `CWR(boolResult)` - Check Win32 BOOL, convert to HRESULT
- `CWRA(boolResult)` - Check Win32 BOOL with assert
- `CHREx(hr, newHr)` - Check HRESULT, replace with custom error code
- `BAIL_OUT_IF(condition, hr_value)` - Early exit with specific HRESULT

#### Variable Declaration with EHM
Because EHM uses `goto`, all variables must be declared before any potential jump:

```cpp
HRESULT MyFunction()
{
    HRESULT hr = S_OK;
    
    // ALL variables declared upfront
    ComPtr<ID3D11Device>        device;
    ComPtr<IDXGIFactory>        factory;
    D3D11_TEXTURE2D_DESC        texDesc  = {};
    DWORD                       flags    = 0;
    
    
    hr = CreateDevice (&device);
    CHRA (hr);
    
    // If you need a local variable mid-function, create a new scope:
    {
        ComPtr<ID3D11Texture2D> tempTexture;
        
        hr = device->CreateTexture2D (&texDesc, nullptr, &tempTexture);
        CHRA (hr);
    }
    
Error:
    return hr;
}
```

#### Function Return Types with EHM
- Most functions using EHM should return `HRESULT` directly (not `bool`)
- This allows error codes to propagate up the call stack
- Calling code checks the result with `CHR()` or `CHRA()`
- Only use `bool` returns for functions that truly represent success/failure without needing error codes

#### Additional Error Handling
- Use `ASSERT()` for debug-only invariant checks
- Use `CBR`/`CBRA` for runtime checks on expected conditions
- Use `BAIL_OUT_IF(condition, hr_value)` for early exit with specific HRESULT

---

## Project-Specific Code Patterns

### Function Header Comments
- Use exactly 80-character wide comment separators with `//` filling
- Format:
```cpp
////////////////////////////////////////////////////////////////////////////////
//
//  ClassName::FunctionName
//
//  [Optional brief description]
//
////////////////////////////////////////////////////////////////////////////////
```
- **ALWAYS** include blank line after closing separator
- Description line often left blank in this codebase

### Variable Declaration Alignment
- Variables at function start must be column-aligned
- Align by: type, pointer/reference symbols, variable name, `=`, and initialization value
- Pointer/reference symbols (`*`, `&`) must be separated from both the type and variable name with a space
- The `*` or `&` is its own column for alignment purposes
- If some variables have pointers/references and others don't, use an empty column to maintain spacing
- **Const placement**:
  - When modifying the type: `const` goes to the left of the type (e.g., `const int`)
  - Treat `const Type` as the type column - don't align `const` as its own column
  - When modifying the pointer: `const` goes in the pointer/reference column (e.g., `* const`)
  - The pointer column contains both the symbol and `const` with one space between (e.g., `* const`)
- Example:
```cpp
HRESULT              hr       = S_OK;
BOOL                 fSuccess = FALSE;
LPWSTR       *       ppszEnd  = nullptr;
DWORD                mode     = 0;
const int            value    = 42;
int          * const pInt     = &value;
const WCHAR  *       pszName  = L"test";
```
- **NEVER** break this alignment when editing

### Naming Conventions

#### Hungarian Notation (TCDir Project)
This project uses Hungarian notation consistently. **ONLY** use Hungarian notation if:
1. The existing codebase already uses it, OR
2. It's a brand new project where you're establishing conventions

**Otherwise**, follow the naming conventions present in the existing codebase.

For projects using Hungarian notation (like TCDir), follow these prefixes:
- `psz` - pointer to string (zero-terminated)
- `sz` - static string buffer
- `cch` - count of characters
- `cb` - count of bytes
- `c` - count/number of items
- `f` - flag/boolean
- `p` - pointer
- `w` - WORD (16-bit)
- `dw` - DWORD (32-bit)
- `n` - numeric value (int)
- `uli` - ULARGE_INTEGER
- `rg` - array ("range")
- `idx` - index
- `ch` - character
- `k` - constant (e.g., `kszDirSize`, `s_krgAttrMap`)
- `s_` - static variable
- `m_` - member variable
- `g_` - global variable

Examples:
- `LPCWSTR pszFileName` - pointer to string for filename
- `size_t cchBuffer` - count of characters in buffer
- `DWORD dwAttributes` - DWORD for attributes
- `bool fSuccess` - flag for success
- `static constexpr WCHAR kszDirSize[]` - constant string
- `int m_cFilesFound` - member variable count

### Static Constant Arrays
Use `static constexpr` for constant lookup tables:
```cpp
static constexpr SFileAttributeEntry s_krgAttrMap[] =
{
    {  FILE_ATTRIBUTE_READONLY, L'R' },
    {  FILE_ATTRIBUTE_HIDDEN,   L'H' },
};
```

### Constructor Initialization Lists
Column-align member initializers:
```cpp
CDirectoryLister::CDirectoryLister (...) :
    m_cmdLinePtr        (pCmdLine),
    m_consolePtr        (pConsole),
    m_configPtr         (pConfig),
    m_cFilesFound       (0),
    m_cDirectoriesFound (0)
{
```

### Line Wrapping - Separator Placement
**CRITICAL**: When wrapping lines, separators (`:`, `,`) **ALWAYS** go at the **END** of the line, **NEVER** at the beginning of the wrapped line.

**Correct**:
```cpp
CMyClass::CMyClass(int arg1, int arg2) :
    m_member1 (arg1),
    m_member2 (arg2)
{
}

void Function(int arg1,
              int arg2,
              int arg3)
{
}
```

**Wrong**:
```cpp
CMyClass::CMyClass(int arg1, int arg2)
    : m_member1 (arg1)  // WRONG - colon at start
    , m_member2 (arg2)  // WRONG - comma at start
{
}

void Function(int arg1
              , int arg2  // WRONG - comma at start
              , int arg3)
{
}
```

### Include Order in .cpp Files
**ALWAYS** follow this exact pattern:
1. `#include "pch.h"` (always first, alone)
2. **1 blank line**
3. Own header file (e.g., `#include "ClassName.h"`)
4. **5 blank lines total from pch.h include**
5. Other project headers (alphabetically sorted if multiple)
6. **5 blank lines before first code construct**

Example:
```cpp
#include "pch.h"

#include "DirectoryLister.h"

#include "CommandLine.h"
#include "Config.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::CDirectoryLister
```

---

## Precompiled Headers

### File Naming
- Precompiled header files should always be `pch.h` and `pch.cpp`
- Don't use `stdafx.h` and `stdafx.cpp` unless that's an existing pattern in the project

### System Headers in pch.h
- **ALL system headers MUST be included ONLY in pch.h**
- **NEVER** include system headers directly in other source files
- System headers are defined as headers not located within the current project/solution
- System headers use `<>` (e.g., `<windows.h>`, `<vector>`, `<algorithm>`)
- Local/project headers use `""` (e.g., `"MyClass.h"`)

### Organization Within pch.h
- Organize includes in sets based on header type:
  - Windows headers
  - C headers
  - C++ headers
  - DirectX headers (if applicable)
  - Other third-party libraries
- **3 blank lines** separate each set of includes
- Alphabetize includes within each set, with exceptions:
  - `windows.h` MUST be first in the Windows headers set (before alphabetization)
  - Error handling headers like `ehm.h` may be included at the end as exceptions to the "no local headers" rule
- Use `using namespace std;` in `pch.h` to apply globally (project preference)
- **Prefer C versions of headers** when available in both C and C++ (e.g., `stdio.h` not `cstdio`, `math.h` not `cmath`)

### pch.h Example Structure
```cpp
#pragma once



// Windows headers
#include <windows.h>

#include <strsafe.h>



// C headers
#include <assert.h>
#include <math.h>
#include <stdio.h>



// C++ headers
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>



using namespace std;



#include "ehm.h"  // Exception: error handling macros
```

### String Handling
- This project uses wide strings (`wstring`, `LPCWSTR`, `WCHAR`) exclusively
- **ALWAYS** use `L""` prefix for string literals
- Prefer `wstring_view` for read-only string parameters
- Use `StringCchVPrintf` and `StringCchCopyEx` from `<strsafe.h>` for buffer safety
- Use `std::format` for modern string formatting

### Common Utilities
- **Array Size Macro**:
  - In projects using EHM (Error Handling Macros): Use `ARRAYSIZE(array)` (from Windows SDK)
  - In modern C++ (C++17+): Prefer `std::size(array)` 
  - In other VC++ projects: Use `_countof(array)` (VC++ built-in)
- Use `UNREFERENCED_PARAMETER(x)` to suppress unused parameter warnings
- Use `BOOLIFY(x)` to convert to boolean (defined as `(!!x)`) - only in projects that define it

### Range-Based Loops
Prefer modern range-based loops:
```cpp
for (const auto& item : container)
for (const SomeType& entry : s_krgArray)
for (auto [key, value] : map)  // structured binding
```

### Comment Separator Width
- All `//` comment separators are **exactly 80 characters** total width
- Function headers use `////////////////...` to fill 80 characters
- **NEVER** create separators of different widths

### Comment Style
- Prefer `//` style for multi-line comments over `/* ... */`
- Use `//` for inline comments and comment blocks

### Brace Style
- Opening braces go on the **next line** for:
  - Function definitions
  - Class/struct definitions
  - Namespace definitions
  - Control structures (if, for, while, switch, etc.)
- Example:
```cpp
void Function()
{
    if (condition)
    {
        // code
    }
}
```
- Exception: Empty function bodies in headers use `{ }` on the same line
- Exception: Empty function bodies in .cpp files use opening brace on new line, closing brace on line below:
```cpp
// In header:
CMyClass() { }

// In .cpp:
CMyClass::CMyClass()
{
}
```

### Class Member Organization
- Order sections as: `public`, then `protected`, then `private`
- Group related members together (constructors, methods, data members)

### Constructor Initialization
- **Always prefer initialization lists** over assignments within constructor body when possible
- **Even better**: Assign default values within the class declaration itself (C++11+)
- Example preference order:
```cpp
// Best: In-class initialization
class MyClass
{
    int m_value = 0;
};

// Good: Initialization list
MyClass::MyClass() :
    m_value (0)
{
}

// Avoid: Assignment in body (only when initialization list won't work)
MyClass::MyClass()
{
    m_value = 0;  // Avoid this pattern
}
```

### Function Parameter Alignment
- **For short parameter lists**, keep all parameters on one line
- **For lengthy sets of parameters**:
  - First parameter stays on the line where function declaration/definition begins
  - Subsequent parameters aligned in column format (like variables), one per line
  - Parameters left-justified with the **first parameter**, not the left margin or one indent level
- Example:
```cpp
HRESULT LongFunctionName (LPCWSTR  pszFirstParam,
                          DWORD    dwSecondParam,
                          BOOL   * pfThirdParam,
                          size_t   cchBufferSize);
```

### Function Parameter Wrapping (Calls)
- **When wrapping parameters in function calls**, keep the first parameter on the same line as the function name
- **Subsequent parameters** are aligned directly under the first parameter (character-aligned, not indent-level aligned)
- **ALWAYS insert one space before an opening parenthesis** unless the parentheses are empty `()`
- This creates visual clarity showing which parameters belong to which function call
- **Correct** (first parameter on function line, rest aligned under it, space before opening paren):
```cpp
HRESULT hr = D3D11CreateDevice (nullptr,                    // Default adapter
                                 D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
                                 nullptr,                    // No software rasterizer
                                 createDeviceFlags,
                                 featureLevels,
                                 _countof (featureLevels),
                                 D3D11_SDK_VERSION,
                                 &m_device,
                                 &featureLevel,
                                 &m_context);
```
- **Wrong** (no space before opening paren, first parameter on new line):
```cpp
HRESULT hr = D3D11CreateDevice(
    nullptr,                    // Default adapter
    D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
    nullptr,                    // No software rasterizer
    createDeviceFlags,
    featureLevels,
    _countof(featureLevels),
    D3D11_SDK_VERSION,
    &m_device,
    &featureLevel,
    &m_context
);
```

### Namespace Usage
- Use `using namespace std;` in `pch.h` to apply globally (project preference)
- For heavy use of a specific namespace (e.g., `std::filesystem`) within a file, add `using namespace` at file scope in that .cpp file
- For light usage, use explicit namespace qualification (e.g., `std::filesystem::path`)
- **Never** put scoped `using namespace` declarations in `pch.h` (only `std` is global)
- Example in .cpp file:
```cpp
#include "pch.h"

#include "MyClass.h"




namespace fs = std::filesystem;  // OK for heavy filesystem usage in this file
```

### SAL Annotations
- Use SAL annotations (`__in`, `__out`, `__deref_out_z`, etc.) where appropriate for API clarity
- However, prefer modern C++ over manual pointer management when possible:
  - Prefer C++ classes over raw pointers
  - Prefer references to pointers in most cases
  - Use smart pointers when ownership semantics are needed
- SAL usage will be limited due to modern C++ preferences

---

## Communication Rules

### When Explaining Changes
- Be concise and direct
- Explain the "why" not just the "what"
- If you make a mistake, acknowledge it immediately and clearly

### Before Major Changes
- Summarize what files will be modified
- Explain pros/cons if there are trade-offs
- Ask for confirmation if approach is unclear

### When Rules Conflict
- **Formatting rules ALWAYS take priority**
- File modification rules come second
- Code style preferences come third
- When in doubt, **ASK** before proceeding

---

## Visual Studio / Windows Specific

### MSBuild Location
- **ALWAYS** use the full path to MSBuild.exe on this machine:
  - `C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe`
- **NEVER** assume `msbuild` is in PATH
- Use this exact path for all build commands
- **CRITICAL**: This path is Enterprise edition, NOT Insiders - use it exactly as specified

### Build Integration
- Always run build after making changes
- Use `get_errors` to verify specific files
- Fix compilation errors before considering task complete
- Check for both errors (C-codes) and warnings

### Performance
- Prefer Windows Console API (`WriteConsoleW`) over C++ streams for console output
- Use large internal buffers to minimize system calls
- Flush strategically, not after every write

---

## Shell and Terminal Rules

### PowerShell is the Default Shell
- **ALL** terminal windows use PowerShell, not CMD
- **ALWAYS** format commands for PowerShell syntax
- **NEVER** use CMD-style commands (e.g., `dir`, `copy`, `del`)
- Use PowerShell equivalents:
  - `Get-ChildItem` or `ls` instead of `dir`
  - `Copy-Item` or `cp` instead of `copy`
  - `Remove-Item` or `rm` instead of `del`
- Use `&` to invoke executables with spaces in paths (e.g., `& "C:\Program Files\..."`)
- Use PowerShell's `-and`, `-or`, `-not` instead of CMD's `&&`, `||`, `!`

---

## Git Commands - Automation Rules

### **ALWAYS** Use Non-Interactive Git Commands
- **ALWAYS** use `git --no-pager` for commands that might trigger pagination
- **NEVER** run git commands that require human interaction to page through results
- **ALWAYS** limit output with `-n` flags to prevent excessive output

### Command Patterns

#### Correct Usage:
```bash
git --no-pager log --oneline -n 20
git --no-pager log --oneline --graph --all -n 20
git --no-pager show --stat <commit>
git --no-pager diff --stat
git --no-pager diff <file>
git status  # Safe - never paginates
git branch  # Safe - never paginates
```

#### Incorrect Usage (Will Trigger Pager):
```bash
git log        # Bad - will paginate
git show       # Bad - will paginate
git diff       # Bad - will paginate
```

### Best Practices
- Use `--oneline` for compact log output
- Use `-n <number>` to limit log entries
- Use `--stat` for file change summaries instead of full diffs
- **ALWAYS** use `--no-pager` even for commands you think won't paginate
- Test commands locally to ensure they don't require paging

---

## Remember
- **Formatting preservation is non-negotiable**
- **Read-only files must stay read-only**
- **When in doubt, ask before modifying**
- **Quality over speed - take time to get formatting right**

---

*Last Updated: 2025-01-XX*
*These rules apply globally to all projects and conversations*
