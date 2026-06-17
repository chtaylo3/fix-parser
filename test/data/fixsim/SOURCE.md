# Dataset: FIXSIM sample messages

- **Source:** https://www.fixsim.com/sample-fix-messages
- **Description:** Clean FIX 4.2 and FIX 4.4 sample messages from a live FIX
  simulator session, decoded field-by-field on the source page. Covers
  NewOrderSingle (D), ExecutionReport (8), OrderCancelRequest (F),
  OrderCancelReplaceRequest (G), AllocationInstruction (J, includes repeating
  groups), AllocationInstructionAck (P), Logon (A), and Logout (5).
- **Retrieved:** 2026-06-17
- **License:** Public sample data published for educational/validation use by
  fixsim.com. Reproduced here for parser test purposes with attribution.

## Local modifications

- The source page renders fields separated by " | " (pipe with surrounding
  spaces) for readability. Here the separators are normalized to a bare `|`
  (no surrounding spaces) so the files are valid pipe-delimited FIX. Field
  values and order are otherwise verbatim.
- One message per line. `fix44.txt` = FIX.4.4 set, `fix42.txt` = FIX.4.2 set.
