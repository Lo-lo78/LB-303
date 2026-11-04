# LB-303 0.99.2 TEST2.4.67 — Incremental parameter search

The keyboard search in the main Grid (Alt+L) and Value now accepts successive letters/digits as a prefix. It searches the beginning of a name first, then the start of any internal word. Single repeated keys cycle with wraparound; new typing after 1000 ms (or a navigation key) restarts the prefix. With Shift held, the search direction is backward, including Shift+digits, which use the digit rather than the printed punctuation symbol. Numeric Edit is not intercepted.

The Parameter Locks picker (Alt+Q > Parameters > Enter) uses the same helper. Search targets the underlying parameter name, ignoring the announcer-only `selected` prefix on existing locks. All normal step/page/transport/preview/preset shortcuts and every existing VST3 ID remain unchanged.

The implementation shares LSampler TEST165's Win32-safe `IncrementalParameterSearch.h` core and `ParameterSearchKey.h` handling. The C++ standalone regression test is run on GitHub Actions following the native Windows Release build. This package contains the complete source; a live REAPER/NVDA check is still required after Windows compilation.
