#include <stddef.h>
#include <stdint.h>

#include <yaml.h>

struct FuzzCursor {
    const uint8_t *data;
    size_t size;
    size_t pos;

    uint8_t NextByte() {
        if (pos >= size) {
            return 0;
        }
        return data[pos++];
    }

    bool NextBool() {
        return (NextByte() & 1) != 0;
    }

    size_t NextRange(size_t n) {
        if (n == 0) {
            return 0;
        }
        return static_cast<size_t>(NextByte()) % n;
    }

    const uint8_t *RemainingData() const {
        if (pos >= size) {
            return data + size;
        }
        return data + pos;
    }

    size_t RemainingSize() const {
        if (pos >= size) {
            return 0;
        }
        return size - pos;
    }
};

static bool EmitOwned(yaml_emitter_t *emitter, yaml_event_t *event) {
    if (yaml_emitter_emit(emitter, event)) {
        return true;  // emitter consumed the event
    }
    yaml_event_delete(event);  // clean up if emitter rejected it
    return false;
}

static yaml_scalar_style_t ChooseScalarStyle(FuzzCursor &cur) {
    switch (cur.NextRange(4)) {
        case 0:
            return YAML_PLAIN_SCALAR_STYLE;
        case 1:
            return YAML_SINGLE_QUOTED_SCALAR_STYLE;
        case 2:
            return YAML_DOUBLE_QUOTED_SCALAR_STYLE;
        default:
            return YAML_LITERAL_SCALAR_STYLE;
    }
}

static yaml_sequence_style_t ChooseSequenceStyle(FuzzCursor &cur) {
    return cur.NextBool() ? YAML_FLOW_SEQUENCE_STYLE : YAML_BLOCK_SEQUENCE_STYLE;
}

static yaml_mapping_style_t ChooseMappingStyle(FuzzCursor &cur) {
    return cur.NextBool() ? YAML_FLOW_MAPPING_STYLE : YAML_BLOCK_MAPPING_STYLE;
}

static bool EmitAlias(yaml_emitter_t *emitter, yaml_char_t *anchor) {
    yaml_event_t event;
    if (!yaml_alias_event_initialize(&event, anchor)) {
        return false;
    }
    return EmitOwned(emitter, &event);
}

static bool EmitScalar(yaml_emitter_t *emitter,
                       FuzzCursor &cur,
                       yaml_char_t *anchor,
                       bool allow_tag) {
    yaml_event_t event;

    static yaml_char_t kStrTag[] = "tag:yaml.org,2002:str";
    yaml_char_t *tag = nullptr;

    if (allow_tag && cur.NextBool()) {
        tag = kStrTag;
    }

    const uint8_t *payload = cur.RemainingData();
    size_t payload_size = cur.RemainingSize();

    // Use a bounded slice so values are varied but not always huge.
    size_t len = cur.NextRange(payload_size + 1);
    yaml_scalar_style_t style = ChooseScalarStyle(cur);

    if (!yaml_scalar_event_initialize(
            &event,
            anchor,
            tag,
            const_cast<yaml_char_t *>(reinterpret_cast<const yaml_char_t *>(payload)),
            static_cast<int>(len),
            1,  // plain_implicit
            1,  // quoted_implicit
            style)) {
        return false;
    }

    return EmitOwned(emitter, &event);
}

static bool EmitNode(yaml_emitter_t *emitter,
                     FuzzCursor &cur,
                     int depth,
                     yaml_char_t *anchor_a,
                     yaml_char_t *anchor_b);

static bool EmitSequence(yaml_emitter_t *emitter,
                         FuzzCursor &cur,
                         int depth,
                         yaml_char_t *anchor,
                         yaml_char_t *anchor_a,
                         yaml_char_t *anchor_b) {
    yaml_event_t event;
    yaml_char_t *tag = nullptr;

    static yaml_char_t kSeqTag[] = "tag:yaml.org,2002:seq";
    if (cur.NextBool()) {
        tag = kSeqTag;
    }

    if (!yaml_sequence_start_event_initialize(
            &event,
            anchor,
            tag,
            1,
            ChooseSequenceStyle(cur))) {
        return false;
    }

    if (!EmitOwned(emitter, &event)) {
        return false;
    }

    size_t count = 1 + cur.NextRange(3);  // 1..3 items
    for (size_t i = 0; i < count; i++) {
        if (!EmitNode(emitter, cur, depth + 1, anchor_a, anchor_b)) {
            return false;
        }
    }

    if (!yaml_sequence_end_event_initialize(&event)) {
        return false;
    }
    return EmitOwned(emitter, &event);
}

static bool EmitMapping(yaml_emitter_t *emitter,
                        FuzzCursor &cur,
                        int depth,
                        yaml_char_t *anchor,
                        yaml_char_t *anchor_a,
                        yaml_char_t *anchor_b) {
    yaml_event_t event;
    yaml_char_t *tag = nullptr;

    static yaml_char_t kMapTag[] = "tag:yaml.org,2002:map";
    if (cur.NextBool()) {
        tag = kMapTag;
    }

    if (!yaml_mapping_start_event_initialize(
            &event,
            anchor,
            tag,
            1,
            ChooseMappingStyle(cur))) {
        return false;
    }

    if (!EmitOwned(emitter, &event)) {
        return false;
    }

    size_t pairs = 1 + cur.NextRange(3);  // 1..3 pairs
    for (size_t i = 0; i < pairs; i++) {
        // Keep keys simpler to avoid pathological invalid structures.
        if (!EmitScalar(emitter, cur, nullptr, true)) {
            return false;
        }
        if (!EmitNode(emitter, cur, depth + 1, anchor_a, anchor_b)) {
            return false;
        }
    }

    if (!yaml_mapping_end_event_initialize(&event)) {
        return false;
    }
    return EmitOwned(emitter, &event);
}

static bool EmitNode(yaml_emitter_t *emitter,
                     FuzzCursor &cur,
                     int depth,
                     yaml_char_t *anchor_a,
                     yaml_char_t *anchor_b) {
    // Depth limit to keep event streams valid and efficient.
    bool force_scalar = depth >= 3;

    // Occasionally emit aliases to previously defined anchors.
    if (!force_scalar && cur.NextRange(10) == 0) {
        if (cur.NextBool()) {
            return EmitAlias(emitter, anchor_a);
        }
        return EmitAlias(emitter, anchor_b);
    }

    // Occasionally attach an anchor to this node.
    yaml_char_t *anchor = nullptr;
    if (!force_scalar && cur.NextRange(6) == 0) {
        anchor = cur.NextBool() ? anchor_a : anchor_b;
    }

    size_t choice = force_scalar ? 0 : cur.NextRange(3);

    switch (choice) {
        case 0:
            return EmitScalar(emitter, cur, anchor, true);
        case 1:
            return EmitSequence(emitter, cur, depth, anchor, anchor_a, anchor_b);
        case 2:
        default:
            return EmitMapping(emitter, cur, depth, anchor, anchor_a, anchor_b);
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    yaml_emitter_t emitter;
    yaml_event_t event;

    unsigned char output[16384];
    size_t written = 0;

    if (!yaml_emitter_initialize(&emitter)) {
        return 0;
    }

    yaml_emitter_set_output_string(&emitter, output, sizeof(output), &written);
    yaml_emitter_set_unicode(&emitter, 1);
    yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

    FuzzCursor cur{data, size, 0};

    static yaml_char_t anchor_a[] = "a1";
    static yaml_char_t anchor_b[] = "b2";

    // Stream start
    if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

    // Single document
    if (!yaml_document_start_event_initialize(&event, nullptr, nullptr, nullptr, 1) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

    if (!EmitNode(&emitter, cur, 0, anchor_a, anchor_b)) {
        goto done;
    }

    if (!yaml_document_end_event_initialize(&event, 1) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

    if (!yaml_stream_end_event_initialize(&event) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

done:
    yaml_emitter_delete(&emitter);
    return 0;
}
