# Zstd compression supports an optional pre-trained dictionary

`--compression zstd` without `--zstd-dict` uses standard Zstd. `--compression zstd --zstd-dict <path>` loads a shared dictionary on both Sender and Receiver, trained offline with `zstd --train` on representative payload samples. The two modes are treated as distinct, comparable benchmark configurations. We chose this over dictionary-always or dictionary-never because the performance difference on small IoT messages (64–256 bytes) is significant enough to be a meaningful benchmark axis, and since we control both sides completely, distributing a shared dictionary file is trivial. The Zstd frame standard embeds the dictionary ID and verifies it on decompression automatically — no custom protocol needed.

## Consequences

- Both Sender and Receiver must receive the same dictionary file before a Run. This is a deployment concern documented in the README.
- A Run with `--zstd-dict` and one without are not directly comparable in the same CSV without the `zstd_dict` column in the output schema.
