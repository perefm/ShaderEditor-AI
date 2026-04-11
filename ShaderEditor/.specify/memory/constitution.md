<!--
Sync Impact Report
- Version change: 2.0.0 -> 3.0.0
- Modified principles:
  - I. Windows Compatibility First -> I. Cross-Platform Compatibility First
  - II. Basic Verification Required -> II. Basic Verification Required
  - III. Simple, Consistent UI -> III. Simple, Consistent UI
  - IV. Responsive by Default -> IV. Stable OpenGL Runtime
  - V. Keep Changes Small -> V. Keep Changes Small
- Added sections:
  - Platform Requirements
  - Delivery Rules
- Removed sections:
  - None
- Templates requiring updates:
  - ✅ updated .specify/templates/plan-template.md
  - ✅ updated .specify/templates/spec-template.md
  - ✅ updated .specify/templates/tasks-template.md
  - ⚠ pending .specify/templates/agent-file-template.md review not required for this minimal constitution
  - ⚠ pending .specify/templates/commands/*.md (directory not present in this repository)
- Follow-up TODOs:
  - None
-->
# ShaderEditor Constitution

## Core Principles

### I. Cross-Platform Compatibility First
The application MUST build and run on the supported desktop platforms defined by
the project. New work MUST not introduce platform-specific behavior that breaks
normal use on another supported platform unless the limitation is documented in
the specification. Rationale: this project is a multiplatform desktop app.

### II. Basic Verification Required
Every change MUST include at least one verification method. Automated tests are
preferred, but when they are not practical the change MUST include a short
manual verification procedure. Rationale: even a minimal project needs a clear
way to prove the change works.

### III. Simple, Consistent UI
User-facing changes MUST follow existing labels, layout patterns, and control
behavior unless the specification explicitly calls for a new pattern. Rationale:
consistency is the minimum standard for a usable desktop application.

### IV. Stable OpenGL Runtime
Features that affect rendering MUST preserve a valid OpenGL context lifecycle,
handle shader or resource failures without crashing the app, and avoid freezing
the UI during normal use. Rationale: graphics stability is the minimum technical
requirement for an OpenGL-based application.

### V. Keep Changes Small
Changes SHOULD be scoped so they are easy to review and easy to roll back.
Unrelated cleanup MUST be avoided unless it is necessary for the task.
Rationale: smaller changes reduce risk in a lightweight workflow.

## Platform Requirements

- Specifications and plans MUST identify the supported platforms affected by the
  feature and any required runtime, windowing, driver, or OpenGL assumptions.
- Features that touch rendering MUST state the expected OpenGL behavior, such as
  context creation, shader compilation, resource loading, or frame updates.
- Features that read or write files MUST use platform-safe paths and expected
  desktop behaviors on each supported platform.

## Delivery Rules

1. Define the user-facing behavior and platform assumptions in the spec.
2. Keep the plan short and focused on implementation approach, platform impact,
   OpenGL impact, and verification.
3. Create tasks that include implementation and at least one verification step.
4. Before merge, confirm the app still builds and the changed workflow works on
   the intended supported platforms.

## Governance

This constitution is the baseline policy for the project. Plans, specs, tasks,
and reviews MUST follow it unless an exception is documented in the change.

Amendments MUST update this file and any directly affected templates in the same
change. Versioning follows semantic versioning for governance: MAJOR for
incompatible principle changes, MINOR for added requirements, and PATCH for
clarifications.

Compliance review is lightweight: each change MUST confirm platform
compatibility, verification method, and rendering impact when applicable.

**Version**: 3.0.0 | **Ratified**: 2026-04-11 | **Last Amended**: 2026-04-11
