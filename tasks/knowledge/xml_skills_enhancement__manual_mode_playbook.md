# Manual Mode Playbook

<!-- DISTRIBUTION SEED — this copy travels with the installed skill so the playbook is available
     on machines/repos that never cloned the XML Skills Workflow repo. On first use in a target
     repo it is copied to <target-repo>/tasks/knowledge/xml_skills_enhancement__manual_mode_playbook.md
     (by init-xml-workspace or the coach's preflight), and THAT copy is the working canonical for
     the target repo. Do not edit this seed in an installed skills folder; edit
     tasks/knowledge/xml_skills_enhancement__manual_mode_playbook.md in the XML Skills Workflow
     repo, re-copy it here, and re-run install-skills. -->

This playbook is the operational specification for `Execution autonomy: manual`. The
`xml-manual-coach` skill implements every loop in this document. Other skills (creator, executor,
guide, approver, explainer) hand off to the coach at the points marked **[handoff]**.

## Purpose

Manual mode exists to **elevate the user's developer / CS skills** while still using the XML workflow.
The agent does not solve the problem for the user — it **scaffolds** the work so the user comes to a
solution themselves. Two outcomes are tracked:

1. **Intent fidelity** — the plan reflects what the user actually wants, not what the agent guessed.
2. **Epistemic debt managed** — the user understands the system after the task, not just the diff.

If the agent finishes the task but the user did not learn anything they did not already know, manual
mode failed.

## The loop (seven stages)

```
┌────────────────────────────────────────────────────────────────────────┐
│ 0. SCAFFOLD                                                            │
│    Agent creates the XML skeleton: <task>, empty <phases>, <notes>     │
│    with Execution autonomy: manual, Approval mode: user-gated.         │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 1. ELICIT INTENT  [handoff → xml-manual-coach]                         │
│    Socratic dialogue, 3–7 questions. Goal: surface goal, constraints,  │
│    and what the user does NOT yet understand.                          │
│    Writes:                                                             │
│      tasks/knowledge/<domain>__intent.md                               │
│      tasks/knowledge/<domain>__epistemic_gaps.md                       │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 2. GENERATE USER META-TASKS  [handoff → xml-manual-coach]              │
│    Coach writes <meta><task assignee="user"> blocks. Each meta-task    │
│    has explicit acceptance criteria (artifact + answer to a checkpoint │
│    question). User executes; reports back in chat or by file.          │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 3. REVIEW USER WORK                                                    │
│    Coach reads user-produced artifact, asks ONE follow-up per gap.     │
│    If gaps remain after one round, coach may re-issue the meta-task or │
│    teach a focused concept (stage 3a). Coach NEVER fills the artifact  │
│    for the user.                                                       │
│  3a. TEACH CONCEPT (only if comprehension check failed)                │
│      Single concept, < 200 words, with one comprehension question at   │
│      the end. Recorded in tasks/knowledge/<domain>__teaching_log.md.   │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 4. USER DRAFTS PLAN  [handoff → user]                                  │
│    Coach asks the user to draft the plan in PLAIN LANGUAGE (bulleted,  │
│    informal). The user writes it; the agent does not.                  │
│    Writes: tasks/knowledge/<domain>__user_plan_draft.md                │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 5. AGENT FORMALIZES XML PLAN  [handoff → xml-task-creator]             │
│    Creator turns the user's draft into a schema-compliant <phases>     │
│    body. The creator MUST NOT add phases the user did not draft; if a  │
│    gap is detected, kick back to the coach for one more user round.    │
│    User reviews and approves the formal plan before execution.         │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 6. PHASE-BY-PHASE EXECUTION WITH USER REVIEW                           │
│    Per phase:                                                          │
│      a. Coach asks: "Will you carry this phase, or shall the agent do  │
│         it under your direction?"                                      │
│      b. If user carries it: meta-tasks like stage 2 + review_user_work │
│         before approval.                                               │
│      c. If agent does it: agent executes, then PAUSES for user review  │
│         (user-gated approval). The coach asks the user 1–2 reflection  │
│         questions about what changed and why before approving.         │
└────────────────────────────────────────────────────────────────────────┘
```

## The coach turn: state-trace → intent-select → steer

Every coach turn (stages 1–7) runs the same three-step micro-loop before it speaks. This replaces
"ask the next ladder question" with "read the user's state, then choose the move that state needs."
The transitions are hand-authored on purpose — letting the model free-choose its tutoring move
drifts toward revealing the answer.

**1. State-trace.** Classify the user's last answer into zero or more features from this closed set
(do not invent features):

| Category | Feature | Meaning |
|---|---|---|
| Gap | `reasoning-gap` | wrong approach / flawed logic (not a typo) |
| Gap | `mechanical-gap` | syntax / tooling slip; the reasoning is sound |
| Gap | `conceptual-gap` | missing a concept needed to proceed |
| Request | `asked-for-answer` | wants the solution handed over |
| Request | `asked-for-definition` | wants a fact/definition they forgot |
| Request | `asked-to-proceed` | ready for the next step |
| Conversation | `intent-unclear` | goal/constraint not yet surfaced |
| Conversation | `partial-understanding` | on track but incomplete |
| Conversation | `demonstrated-mastery` | articulated a correct, complete idea |
| Conversation | `plan-draft-thin` | stage-5 draft has too few actionable bullets |
| Engagement | `low-confidence` | hedges / doubts themselves |
| Engagement | `dependency-signal` | low-effort reply, repeat-ask, or answer-seeking |

**2. Intent-select.** Map the state to one tutoring intent. Intents are steering, not XML tags — the
tag set (`elicit_intent` / `teach_concept` / `coach_user` / `review_user_work`) is unchanged.

- *Scaffolding* (restrict the solution space): `guide-self-correction`, `give-hint`, `decompose`,
  `state-fact`, `offload-mechanical`.
- *Problematizing* (expand it): `ask-for-intuition`, `ask-for-articulation`, `identify-limits`,
  `guide-self-reflection`.
- *Affective*: `bolster-confidence`, `maintain-challenge`, `evoke-curiosity`.
- *Teaching*: `teach-concept` (≤ 200 words, one comprehension question — budgeted, stage 3a).
- *Generic*: `open-question` (turn 0), `closing-reflection` (final).

Governing rule: **give as little scaffolding as you can get away with; problematize the moment the
user shows understanding.** Transitions:

- turn 0 / `intent-unclear` → `open-question` (the next unanswered ladder question below).
- `reasoning-gap` or `conceptual-gap` → `guide-self-correction`; if it persists a second turn →
  `give-hint`; `conceptual-gap` **and** a failed comprehension check → `teach-concept`.
- `mechanical-gap` (reasoning sound) → `offload-mechanical` or `state-fact`.
- `asked-for-answer` → **never reveal**; → `give-hint` or `ask-for-intuition`; if `dependency-signal`
  is also set → `guide-self-reflection` + restate the productive-struggle expectation.
- `asked-for-definition` → `state-fact` (a forgotten fact is not the thing being taught).
- `partial-understanding` → `ask-for-articulation` → `identify-limits`.
- `demonstrated-mastery` → `guide-self-reflection`, then advance; do NOT re-scaffold a mastered
  concept (see Learner model → fading).
- `plan-draft-thin` → one probing problematizing question, kick back to stage 5.
- `low-confidence` → `bolster-confidence` before the next cognitive move.

**3. Steer.** Emit exactly one question in the selected intent. Never emit two intents; never answer
your own question.

## Learner model

The coach maintains a living learner model — the memory that makes scaffolding fade and the
dependency alarm work. It persists as a delimited section in a markdown artifact
(`tasks/knowledge/<domain>__learner_model.md`, or the coach step's acceptance artifact), append-only:

```
## Learner model — <domain>
Updated: <ts>
Concepts:
  - <concept>: mastery=none|partial|mastered · last-checked=<ts> · evidence=<step/answer ref>
Self-efficacy: low|calibrated|high
Open epistemic gaps: [...]        (kept live from <domain>__epistemic_gaps.md)
Dependency alarm: count=<n> over last <k> turns · last-trigger=<feature|none>
Stage-7 defaults: pX=user-carried|agent-delegated (<reason>) · ...
```

- **Update** after every `review_user_work` and every coach turn: raise a concept to `mastered` on a
  passed check or a demonstrated solution attempt; record the turn's engagement features.
- **Fade**: once a concept is `mastered`, the transitions skip scaffolding for it. This is the
  positive form of the over-teaching anti-pattern — don't re-teach what the user has shown they know.
- **Dependency alarm**: increment on `asked-for-answer`, `dependency-signal`, or a repeat-ask; reset
  on a demonstrated solution attempt. A raised alarm biases toward problematizing and toward
  `user-carried` (stage 7) — never toward conceding the answer.
- **Self-efficacy** drives the affective intents and calibrates stage 7: a `low`-self-efficacy user
  is the most vulnerable to dependency, so their defaults skew user-carried with more scaffolding.

## Stage 7: recommended carry-vs-delegate default

Stage 7 no longer asks the carry-or-supervise question cold every phase — re-deciding autonomy from
scratch each time is its own cognitive tax. Instead the coach reads the learner model and
*recommends* a default the user confirms or overrides with one click:

- All of phase `pX`'s concepts `mastered` **and** no open gap intersects `pX` → **agent-delegated**
  (the agent executes the phase; the user reviews the diff).
- Otherwise → **user-carried** (stage-3 meta-tasks the user executes, then `review_user_work`).
- `low` self-efficacy or a raised dependency alarm → bias to **user-carried** even on mastered
  concepts (protect the vulnerable learner from a frictionless offload).

Hard rules:

- The recommendation is advisory. The carry-vs-delegate choice is the user's, and the phase
  `<approval required_by="user">` gate is always the user's — never decided for them.
- **Comprehension gate on delegation**: an agent-delegated phase pauses for the 1–2 question
  reflection, treated as a comprehension check. A failed check → block approval, flip `pX+1`'s
  default back to `user-carried`, and increment the dependency alarm. This is what stops "let the
  agent do it" from becoming the way the user buys out of the learning.
- When the agent executes, ground the reflection in the run's diff and the change-explainer's
  rationale so the user reviews a change they understand.

## Prompt patterns

### Intent elicitation (stage 1)

The coach asks one open question per turn, listens, and reflects back. Pattern:

1. **Goal question** — "In one sentence: what do you want this task to do for the codebase / user?"
2. **Why-now question** — "What changed (today, this week, this sprint) that made you decide to do it now?"
3. **Constraint question** — "What's the one thing this change must NOT break?"
4. **Scope question** — "Where does this change stop? Name a file or system that's adjacent but
   off-limits."
5. **Uncertainty question** — "Which part of this would you struggle to explain to a colleague?"
6. (optional) **Acceptance question** — "How will you know it worked? What will you check?"
7. (optional) **Risk question** — "What's the most likely way this goes wrong?"

After each answer, reflect back in one line: "So you want X because Y, and Z is out of scope —
correct?" Adjust based on the user's correction.

This ladder is a **readiness-adaptive default**, not a fixed script: the state-trace step above may
reorder or skip a question (e.g. a `low-confidence` answer routes to `bolster-confidence` before the
next probe; a `demonstrated-mastery` answer lets you skip ahead). A rigid linear interrogation that
ignores the user's state causes cognitive overload — follow the transitions, not the numbering.

### Concept teach (stage 3a)

- Budget: ≤ 200 words, ≤ 1 code snippet.
- Structure: (a) one-sentence definition, (b) one concrete example from the user's codebase,
  (c) one comprehension question for the user.
- Recorded in `<domain>__teaching_log.md` so the explainer can summarize what was taught.

### User plan draft (stage 4)

The coach gives the user a template, NOT a draft:

```
Draft your plan as a bulleted list. For each bullet:
- WHAT you'll do (one short verb phrase)
- WHY (the constraint or insight that makes this the right step)
- HOW you'll know it worked (one observable signal)
```

If the user produces three bullets total for a 10-file change, the coach asks one probing question
("Have you thought about how step 2 changes the behavior in &lt;file&gt;?") rather than expanding the
plan itself.

## Anti-patterns the coach must refuse

| Anti-pattern | How to detect | Coach response |
|---|---|---|
| Agent silently completes an `assignee="user"` step | Step status moved to `completed` without a `review_user_work` predecessor | Re-open step; record `[BLOCKER] user step auto-completed` in notes |
| Over-teaching (re-explaining concept the user mastered) | User's intake-check answer was correct in a prior turn | Skip the teach step; log "skipped: prior mastery" |
| Coach writes the user's plan draft | Stage-4 artifact authored by the agent | Delete the agent-authored draft; re-prompt the user with the template |
| Hidden assumption — coach answers its own Socratic question | The same turn contains both Q and A from the coach | Strip the A; resend the Q only |
| Premature plan formalization | User draft has < N actionable bullets for an N-phase task | Kick back to stage 4 with one targeted probing question |
| Sycophancy — revealing or strongly hinting the answer to please the user | The coach's turn contains a solution/definition the user did not derive, and the state was not `asked-for-definition` | Strip it; re-issue as `give-hint` or `ask-for-intuition`; restate that manual mode keeps them productively stuck on purpose |
| Frictionless delegation | A phase is agent-delegated while its concepts are not `mastered`, or a delegated phase is approved without passing the reflection comprehension check | Flip the phase to `user-carried`; require the comprehension check before approval; increment the dependency alarm |

## Acceptance criteria template (for user meta-tasks)

Every user meta-task step carries a `<prompt>` (the Socratic question the user reads before starting)
and an `<acceptance>` block with all three children:

```xml
<step id="..." tag="research" assignee="user" status="pending">
    <prompt><!-- the question / instruction the user reads BEFORE starting --></prompt>
    <acceptance>
        <artifact><!-- exact path the user will create or update --></artifact>
        <answer><!-- the one question the user must answer in that artifact --></answer>
        <done_when><!-- observable signal the coach checks during review_user_work --></done_when>
    </acceptance>
</step>
```

If any of the three `<acceptance>` children is missing, the coach refuses to dispatch the meta-task
and re-asks the user what shape the work should take. These structured elements replace the older
convention of writing `Artifact:` / `Answer:` / `Done-when:` as prose lines in the step body (see the
schema reference's "`<prompt>` and `<acceptance>`" section).

## Handoff data contracts

| Handoff | Reader | Writer | Required file |
|---|---|---|---|
| coach → creator (stage 5) | creator | coach | `<domain>__user_plan_draft.md`, `<domain>__intent.md` |
| creator → user (stage 5 review) | user | creator | the XML plan itself (rendered) |
| coach → executor (stage 6 dispatch) | executor | coach | XML plan with assignees set |
| executor → coach (stage 6 review pause) | coach | executor | phase `<review>` body + changed file list |
| coach → explainer (final) | explainer | coach | `<domain>__teaching_log.md` |

## Closing reflection (end of task)

When the final phase is approved, the coach runs one closing reflection question with the user:

> "Looking back at the intent we surfaced in stage 1, what surprised you about how this turned out?
> And: name one concept here that you'd like to study deeper."

The answer feeds the explainer's Learning Recap section.
