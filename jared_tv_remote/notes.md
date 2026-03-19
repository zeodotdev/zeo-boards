# Zeo MCP Tool Interface — Review Notes

Notes from reviewing and fixing a solar-powered universal TV IR remote project (ATtiny84A, 3 hierarchical sheets). Focus: what was hard, confusing, or friction-y about the Zeo tool interface.

---

## What Worked Well

### `check_status` is a great starting point
Returns open editors, project path, and file paths immediately. Clear and concise — no ambiguity about what's loaded.

### `screenshot` is invaluable
Being able to see the actual schematic visually is critical for catching things that raw data misses — like component overlap, label positioning issues, and general layout quality. The screenshots rendered quickly and were high quality.

### `sch_get_summary` gives a good high-level view
The audit section is especially useful — it flags orphaned labels, power symbols, and junctions immediately, which saved time diagnosing issues. Counts are helpful for sanity-checking.

### `sch_run_erc` with `by_type` then `detailed` is a good workflow
The `by_type` grouping gives a quick triage view, then `detailed` provides the specifics needed to locate and fix issues.

### `sch_label_pins` is the right tool for labeling
Once I switched from manual `sch_add` to `sch_label_pins`, placing 11 hierarchical labels on the MCU was a single call with zero placement failures. It handles position, direction, and justification automatically.

### `sch_delete` with query objects works reliably
Deleting by type+text (e.g., `{type: "global_label", text: "ROW0"}`) was clean and predictable. The `cleanup_wires: false` option was useful when I wanted to preserve wiring during label swaps.

---

## Pain Points / Struggles (numbered chronologically)

### 1. Navigating to the root sheet is unintuitive
I tried `sch_switch_sheet` with `"/"` (which the tool docs suggest for root), but it failed with "Sheet not found." I had to call it with no arguments first to discover the actual root sheet name (`"3 - complex pcb"`), then navigate using that. The root sheet path turned out to be `/3 - complex pcb/` not `/`.

**Suggestion:** Make `"/"` work as an alias for the root sheet, or document the actual behavior more clearly.

### 2. Hard to tell what's INSIDE a sheet vs what's ON the sheet symbol
The root sheet's `sch_get_summary` shows sheets with `pin_count` (e.g., Peripherals has 11 pins). But it's not obvious whether those pins correspond to hierarchical labels inside the sub-sheet or are orphaned sheet pins from a previous label strategy. I had to navigate into each sheet and cross-reference the label types (local vs global vs hierarchical) to understand the connectivity architecture.

**Suggestion:** Show sheet pin names alongside pin count, and whether they have matching hierarchical labels inside.

### 3. No way to distinguish label types without reading the full summary
In the Peripherals sheet, the labels show `"type": "LocalLabel"`. In the MCU sheet, they show `"type": "GlobalLabel"`. But when you're looking at ERC errors like "Same local/global label," you have to manually correlate which sheet has which type. The ERC detailed output only gives positions and UUIDs — not the label text or type for each violation.

**Suggestion:** ERC detailed violations should include the actual label text and type, not just positions and item IDs.

### 4. Power Supply sheet issues were hard to diagnose from data alone
The audit flagged 5 orphaned power symbols by reference (e.g., `#PWR011`), but power symbol references are auto-generated and meaningless. I couldn't tell which GND or PWR_FLAG was the problem without cross-referencing positions with the screenshot.

**Suggestion:** For orphaned power symbols, include the value (GND, +3V3, PWR_FLAG) and the nearest component/label as context.

### 5. ERC violations reference item UUIDs that aren't exposed elsewhere
The `detailed` ERC output includes `item_ids` (UUIDs), but `sch_get_summary` returns symbol refs and label text — not UUIDs. There's no way to map an ERC violation UUID back to a specific component without comparing positions.

**Suggestion:** Either include UUIDs in `sch_get_summary` output, or include refs/label text in ERC violations.

### 6. No bulk inspection across sheets
To understand the full schematic, I had to: switch → screenshot → summary → switch to next. For a 3-sheet project = 12 tool calls. For 10+ sheets this would be very slow.

**Suggestion:** A `sch_get_summary` option to return all sheets at once, or a project-level aggregate.

### 7. `sch_switch_sheet` doesn't return the new sheet's summary
After switching sheets, you always need follow-up `sch_get_summary` and/or `screenshot` calls. That's 2-3 round trips per sheet navigation.

**Suggestion:** Optionally include the target sheet's summary in the switch response.

### 8. Sheet pins are invisible to the API and can't be deleted
The Peripherals sheet symbol on root had 11 orphaned sheet pins. These are visible in screenshots and `sch_get_summary` reports `pin_count: 11`, but:
- `sch_inspect` on `sheets` doesn't list them
- `sch_inspect` with `section: "all"` doesn't show them
- `sch_delete` has no `type: "sheet_pin"` query
- `sch_label_pins` on the sheet name didn't help

I literally could not enumerate or delete these sheet pins through any available tool. This made fixing the hierarchical label mismatch much harder.

**Suggestion:** Add sheet pin visibility to `sch_inspect` sheets section, and add `type: "sheet_pin"` to `sch_delete` queries.

### 9. No way to change a label's type (local → global → hierarchical)
`sch_update` can modify position, rotation, mirror, and properties — but NOT label type. To change a local label to hierarchical, you must delete and recreate it. For 33 labels, that's a lot of round trips.

**Suggestion:** Allow `sch_update` to change label type.

### 10. `sch_add` label overlap detection is too aggressive and error messages are vague
When placing hierarchical labels at the endpoint of a wire (placed in the same batch), every label was rejected with "Overlaps a wire segment." The error doesn't say which element it overlaps or in what direction the label body extends.

I tried:
- Labels at the left end of horizontal wires → "Overlaps a wire segment"
- Labels at the right end → "Overlaps existing element(s)"
- More spacing → still "Overlaps a wire segment"

I couldn't figure out the correct placement from the errors alone. There's no documentation on which direction hierarchical label flags extend.

**Suggestion:** (a) Allow labels at wire endpoints in the same batch, (b) error messages should specify WHAT was overlapped (wire UUID, component ref, or another label), (c) document the label flag geometry.

### 11. Labels reported as "Placement rejected" were actually placed successfully
In my second `sch_add` attempt, all 11 labels were reported as failed with "Placement rejected: Overlaps a wire segment." But when I later inspected the sheet, 10 of the 11 hierarchical labels were actually placed at the correct positions. Only STATUS_LED (which genuinely overlapped C1) truly failed.

This is a significant bug — I wasted 3+ iterations trying to work around "failures" that didn't exist. The tool told me the labels weren't placed, so I deleted the wires and tried again, creating more mess.

**Suggestion:** Fix the return status to accurately reflect what was placed.

### 12. Tool instructions contradict the plan (global vs hierarchical labels)
The Zeo system prompt says: "Use `label_type: 'hierarchical'` for inter-sheet signals. Never use global labels unless you have a specific reason." But the plan.md (written by a previous agent session) said "global labels for inter-sheet connections."

I initially followed the plan and tried to convert everything to global labels. A human had to catch the contradiction. The previous agent that built this schematic also used global labels on the MCU sheet, violating the tool's own guidance.

**Suggestion:** This is partly a prompt issue — the instructions about hierarchical vs global labels should be more prominent or enforced. Consider warning when global labels are placed.

### 13. `sch_get_summary` didn't show hierarchical labels
When I first inspected the Peripherals sheet, `sch_get_summary` returned 33 labels — all LocalLabel type. But `sch_inspect` later revealed 10 HierarchicalLabels also existed on the sheet. The summary silently omitted them, which led me to believe there were no hierarchical labels inside the sheet (reinforcing the wrong diagnosis).

**Suggestion:** `sch_get_summary` should include all label types, or at minimum note that hierarchical labels exist.

### 14. Can't place local labels near hierarchical labels on the same wire
I needed to bridge hierarchical labels to existing local label nets. Tried placing local labels at the endpoint of wires attached to hierarchical labels (5.08mm spacing between labels). Every attempt was rejected: "Overlaps 'COL0' by 0.7mm horizontal, 0.3mm vertical." Even with 10mm wires and 5mm spacing, the label bounding boxes still collide.

This raises a design question: do hierarchical labels and local labels with the same name on the same sheet automatically share a net? If so, the local labels aren't needed and the tool should document this. If not, there's no way to bridge them without a very long wire to separate the label bodies.

**Suggestion:** Document whether same-name hierarchical + local labels share a net. If they do, no bridging is needed. If not, provide a way to connect them (e.g., `sch_connect_net` should support label-to-label connections).

### 15. `sch_delete` with type+text+position doesn't match labels at exact coordinates
Tried to delete the orphaned STATUS_LED local label using `{type: "label", text: "STATUS_LED", position: [22.86, 102.87]}` — the exact coordinates from `sch_inspect`. Got "queries_not_matched." Had to fall back to deleting by UUID instead.

The position matching should work when the coordinates come directly from a prior inspect call. Position tolerance is documented as 0.01mm, but even exact matches fail.

**Suggestion:** Fix position matching for labels, or provide clearer error when a position-based query fails (e.g., "nearest label 'STATUS_LED' is at [x, y], 0.5mm away").

---

## Schematic-Specific Observations (for context)

The schematic had a fundamental label architecture mismatch — MCU used global labels while Peripherals used local labels + hierarchical labels (partially connected). The Power Supply had multiple orphaned power symbols and dangling wires. The circuit topologies themselves were correct and matched the plan.

Fix approach (in progress):
1. MCU sheet: replaced global labels with hierarchical labels via `sch_label_pins` ✅
2. Peripherals sheet: 10 hierarchical labels placed with wire stubs, STATUS_LED orphaned label deleted, need to add STATUS_LED hierarchical label (in progress)
3. Power Supply: wiring fixes (pending)
4. Root sheet: wire sheet pins together (pending)
