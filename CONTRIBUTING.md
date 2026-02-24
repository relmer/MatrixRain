# Contributing to MatrixRain

## Git Commit Messages

This project uses [Conventional Commits](https://www.conventionalcommits.org/).

### Format

```
type(scope): description

optional body
```

- **type** and **description** must be lowercase
- **description** uses imperative mood, no trailing period
- **scope** is optional but preferred

### Types

| Type | Purpose |
|------|---------|
| `feat` | New feature |
| `fix` | Bug fix |
| `refactor` | Code restructuring (no behavior change) |
| `perf` | Performance improvement |
| `test` | Adding or correcting tests |
| `docs` | Documentation only |
| `style` | Formatting (no code change) |
| `build` | Build system or dependency changes |
| `ci` | CI configuration changes |
| `chore` | Other changes (no src or test modification) |

### Scopes

Common scopes for this project:

- `core` - MatrixRainCore library (application logic, character systems)
- `rendering` - DirectX 11 rendering, shaders, bloom
- `animation` - Animation system, streaks, density control
- `registry` - Windows registry settings
- `tests` - Test project
- `ui` - Config dialog, windowing

### Body

- Explain **what** and **why**, not how
- Wrap at 72 characters
- Use bullet points (`-`) for multiple changes

### Examples

```
fix(core): improve thread safety and correctness

- Extend mutex scope to cover Render() call in game loop
- Use thread_local RNG to eliminate shared mutable state

refactor(core): extract shared color scheme parsing

perf(animation): reuse temp vectors in RemoveExcessStreaks
```
