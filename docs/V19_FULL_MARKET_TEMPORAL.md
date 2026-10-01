# v19: Full-market temporal validation

This version ports the v18 five-session temporal protocol into the v16 12-trader CDA, preserving the v15 generic scientist and earlier evidence components. Each session executes a matched control and treatment market using the same activation and quote random variates. The target agent's adaptive memory is carried between treatment sessions; other traders and the market order book reset. Training centroids are frozen before a disjoint held-out seed partition is evaluated. No post-hoc threshold changes are made to pass gates.

**Finding:** The controlled policy probe's v18 perfect mechanism recovery did not survive nonlinear market integration. See V19_MARKET_TEMPORAL_REPORT.md for actual rates. The 80% held-out recovery gate fails, so this is a diagnostic integration build, not a validated general classifier. The null-control gate passes for the tested partition. Next improve observability by capturing every attempted and accepted target quote, quote context and within-session timing; quote averages alone lose mechanism-identifying information.

Run: `cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure && ./build/v19-market-temporal`.
