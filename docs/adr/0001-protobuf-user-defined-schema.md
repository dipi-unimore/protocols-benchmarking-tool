# Protobuf uses a user-defined .proto schema, not a generic bytes wrapper

The Protobuf serializer encodes payload data using a developer-supplied `.proto` file compiled at build time via CMake. Field names in the JSON/YAML payload source must match the proto field names; `JsonStringToMessage` handles the mapping. We rejected a generic `bytes data = 1` wrapper because it would make Protobuf indistinguishable from a raw binary passthrough, eliminating the compression and schema-efficiency advantages that motivate testing Protobuf in the first place. Since we control both Sender and Receiver completely, sharing a compiled schema is straightforward and produces a meaningful, realistic benchmark axis.

## Considered Options

- **Generic `bytes data = 1` wrapper** — no schema required, but Protobuf becomes a no-op wrapper with marginal framing overhead. Not representative of real Protobuf usage.
- **User-defined schema** (chosen) — requires `.proto` file and build-time `protoc` step, but produces realistic wire sizes and enables fair comparison with CBOR and None.
