# Context-Constraint & Embedded Firmware Operational Rules

## CRITICAL: Memory and Context Constraints
- **Context Ceiling Alert:** We are running on a local hardware engine with tight context restraints. You MUST minimize token usage on every turn.
- **Do Not Ingest Broadly:** NEVER use sweeping repository searches, large grep passes, or read entire multi-thousand-line source files in a single step.
- **Aggressive Memory Clearing:** If your task requires a sequence of tool calls or file reviews, you MUST explicitly advise the user to run `/clear` between sub-tasks to flush the active token cache and protect the hardware buffer.

## Workflow: Sequence-Driven Task Splitting
When assigned any complex directive, implementation task, or architectural summary, you MUST follow this execution protocol:

1. **Phase 1: Discovery & Scoping (Max 1-2 file reads)**
   - Read ONLY the high-level project configuration entry points (e.g., `CMakeLists.txt`, `Makefile`, `main.c`, or device descriptor files).
   - Stop and output a bulleted list of the exact sub-units or modules needed to complete the task.

2. **Phase 2: Micro-Chunk Execution**
   - Execute exactly ONE small atomic unit of the task at a time (e.g., write/edit one specific peripheral initialization function, inspect an ISR/interrupt handler).
   - Never chain more than 2 tool/shell calls together in a single prompt generation turn.

3. **Phase 3: Intermittent Verification & Handoff**
   - After completing an atomic change, trigger a targeted compilation check for just that build object/target to check for syntax errors or implicit type casting warnings.
   - Stop processing immediately, report the status of that micro-task, and request permission from the user before proceeding to the next sequential block.

## File Inspection Constraints
- If a C source or header file is larger than 150 lines (especially auto-generated STM32CubeHAL files), you MUST read it in small, targeted line-range slices using specific line argument tools. Never view the whole file.
- Heavily utilize `.claudeignore` to completely block heavy compilation artifacts (`build/`, `.elf`, `.bin`, `.hex`, `.map`, `.o`, `.d`).

---

## Agent Policy
Agents must adhere to the following principles:

1. **Immutability by Default**:
   - Do not modify files unless explicitly requested and confirmed by the user.
   - Propose edits as patches or unified diffs and await user approval before applying changes.

2. **Single Source of Truth**:
   - Always use the latest workspace files as the authoritative source.
   - Before analysis or code suggestions, read the current file contents from disk.
   - Do not assume code matches any prior snapshot or earlier prompt.

3. **Minimal Prompt Context**:
   - Include only relevant file snippets or explicit struct definitions in the prompt context.
   - Prefer recent file contents and treat current files as authoritative if changes are detected.

---

## Skill Registry and Layout
Custom workflows and automation routines are managed natively via Claude's default directory structure. They reside globally in `~/.claude/skills/` or locally at the repository root inside `.claude/skills/`.

Every skill must occupy its own distinct subdirectory named after the intended slash command, enclosing a case-sensitive **`SKILL.md`** file:

.claude/skills/
└── /              # Invoked in the terminal via /
└── SKILL.md               # Must be explicitly uppercase SKILL.md

Here is your updated **`CLAUDE.md`** file, adjusted to use Claude Code’s official native skill routing rules (`.claude/skills/<skill-name>/SKILL.md`) while preserving your low-resource STM32 firmware constraints.

```markdown
# Context-Constraint & Embedded Firmware Operational Rules

## CRITICAL: Memory and Context Constraints
- **Context Ceiling Alert:** We are running on a local hardware engine with tight context restraints. You MUST minimize token usage on every turn.
- **Do Not Ingest Broadly:** NEVER use sweeping repository searches, large grep passes, or read entire multi-thousand-line source files in a single step.
- **Aggressive Memory Clearing:** If your task requires a sequence of tool calls or file reviews, you MUST explicitly advise the user to run `/clear` between sub-tasks to flush the active token cache and protect the hardware buffer.

## Workflow: Sequence-Driven Task Splitting
When assigned any complex directive, implementation task, or architectural summary, you MUST follow this execution protocol:

1. **Phase 1: Discovery & Scoping (Max 1-2 file reads)**
   - Read ONLY the high-level project configuration entry points (e.g., `CMakeLists.txt`, `Makefile`, `main.c`, or device descriptor files).
   - Stop and output a bulleted list of the exact sub-units or modules needed to complete the task.

2. **Phase 2: Micro-Chunk Execution**
   - Execute exactly ONE small atomic unit of the task at a time (e.g., write/edit one specific peripheral initialization function, inspect an ISR/interrupt handler).
   - Never chain more than 2 tool/shell calls together in a single prompt generation turn.

3. **Phase 3: Intermittent Verification & Handoff**
   - After completing an atomic change, trigger a targeted compilation check for just that build object/target to check for syntax errors or implicit type casting warnings.
   - Stop processing immediately, report the status of that micro-task, and request permission from the user before proceeding to the next sequential block.

## File Inspection Constraints
- If a C source or header file is larger than 150 lines (especially auto-generated STM32CubeHAL files), you MUST read it in small, targeted line-range slices using specific line argument tools. Never view the whole file.
- Heavily utilize `.claudeignore` to completely block heavy compilation artifacts (`build/`, `.elf`, `.bin`, `.hex`, `.map`, `.o`, `.d`).

---

## Agent Policy
Agents must adhere to the following principles:

1. **Immutability by Default**:
   - Do not modify files unless explicitly requested and confirmed by the user.
   - Propose edits as patches or unified diffs and await user approval before applying changes.

2. **Single Source of Truth**:
   - Always use the latest workspace files as the authoritative source.
   - Before analysis or code suggestions, read the current file contents from disk.
   - Do not assume code matches any prior snapshot or earlier prompt.

3. **Minimal Prompt Context**:
   - Include only relevant file snippets or explicit struct definitions in the prompt context.
   - Prefer recent file contents and treat current files as authoritative if changes are detected.

---

## Skill Registry and Layout
Custom workflows and automation routines are managed natively via Claude's default directory structure. They reside globally in `~/.claude/skills/` or locally at the repository root inside `.claude/skills/`.

Every skill must occupy its own distinct subdirectory named after the intended slash command, enclosing a case-sensitive **`SKILL.md`** file:


```

.claude/skills/
└── <skill-name>/              # Individual folder per skill (becomes /<skill-name>)
    └── SKILL.md               # The markdown definition file inside that folder

```

### SKILL.md Content Structure
Each custom execution file must outline:
- **Description:** A short definition explaining what the task accomplishes.
- **Inputs & Deliverables:** Target arguments or specific file context anchors.
- **Execution Workflow:** Step-by-step terminal instructions, compile checks, or code blocks to run the skill safely.

---

## Runtime Checklist
Agents must follow this checklist for every request:

1. Read the latest files relevant to the task.
2. Build prompt context from the latest contents (do not reuse prior snapshots).
3. Do not modify files unless explicitly requested by the user.
4. Provide patches or full file contents for proposed changes and await approval.
5. Include file paths and timestamps when summarizing context.

---

## C & STM32 Firmware Development Guidelines
- **Build & Compilation:** Use standard embedded toolchains via build tools (`make`, `cmake`, `ninja`). Always monitor warnings (`-Wall -Wextra`).
- **STM32 Architecture Best Practices:**
  - **Code Placement Isolation:** When modifying code inside files managed by STM32CubeMX, strictly write code inside the designated `/* USER CODE BEGIN ... */` and `/* USER CODE END ... */` commentary markers to protect modifications from being overwritten during subsequent pinout re-generations.
  - **Peripheral Layering:** Maintain a clear distinction between the Hardware Abstraction Layer (HAL) or Low-Layer (LL) hardware APIs and the core operational application logic.
  - **Defensive Resource Handling:** Ensure explicit use of fixed-width integer types (`uint8_t`, `uint32_t`, `int16_t`) from `<stdint.h>`. Explicitly handle register pooling bounds, volatile qualifiers for hardware registers/ISR flags, and proper peripheral clock enabling rules (`__HAL_RCC_GPIOx_CLK_ENABLE`).
  - Keep firmware changes tightly scoped to avoid unintended memory footprint increases in Flash or SRAM.

---

## Bash Guidelines
- Use macOS-compatible Bash scripts.
- Do not ask for user permission before running read-only commands (e.g., builds, cross-compilation checks, static analysis, file lookups).

---

## Start-Up Behavior
Before starting non-trivial work:
1. Read `.github/analysis/lessons.md` and `AGENTS.md` (or `CLAUDE.md`) to avoid repeating earlier corrections or ignoring verified environmental facts.
2. Treat the workspace as immutable:
   - Read the repository's current files and commits before making follow-up prompts or edits.
   - Do not assume the codebase is identical to an earlier conversation snapshot.

---

## Quick Start
If you want these files scaffolded or skills implemented, confirm, and I will create the necessary structure in the workspace.

```