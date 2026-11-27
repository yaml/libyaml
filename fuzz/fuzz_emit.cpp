#include <stddef.h>
#include <stdint.h>

#include <yaml.h>

static bool EmitOwned(yaml_emitter_t *emitter, yaml_event_t *event) {
    if (yaml_emitter_emit(emitter, event)) {
        return true;  // emitter consumed the event
    }
    yaml_event_delete(event);  // clean up on failure
    return false;
}

static bool EmitScalar(yaml_emitter_t *emitter,
                       const uint8_t *data,
                       size_t size,
                       yaml_scalar_style_t style) {
    yaml_event_t event;
    static yaml_char_t tag[] = "tag:yaml.org,2002:str";

    if (!yaml_scalar_event_initialize(
            &event,
            nullptr,                  // anchor
            tag,                      // tag
            (yaml_char_t *)data,      // value
            (int)size,                // length
            1,                        // plain_implicit
            1,                        // quoted_implicit
            style)) {
        return false;
    }

    return EmitOwned(emitter, &event);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    yaml_emitter_t emitter;
    yaml_event_t event;

    unsigned char output[8192];
    size_t written = 0;

    // Declare these before any goto target crossing.
    uint8_t selector = (size > 0) ? data[0] : 0;
    const uint8_t *payload = (size > 0) ? data + 1 : data;
    size_t payload_size = (size > 0) ? size - 1 : 0;

    if (!yaml_emitter_initialize(&emitter)) {
        return 0;
    }

    yaml_emitter_set_output_string(&emitter, output, sizeof(output), &written);
    yaml_emitter_set_unicode(&emitter, 1);
    yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

    // 1. Stream start
    if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

    // 2. Document start
    if (!yaml_document_start_event_initialize(&event, nullptr, nullptr, nullptr, 1) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

    yaml_scalar_style_t scalar_style;
    switch ((selector >> 2) % 3) {
        case 0:
            scalar_style = YAML_PLAIN_SCALAR_STYLE;
            break;
        case 1:
            scalar_style = YAML_SINGLE_QUOTED_SCALAR_STYLE;
            break;
        default:
            scalar_style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
            break;
    }

    switch (selector % 3) {
        case 0: {
            // Single scalar document
            if (!EmitScalar(&emitter, payload, payload_size, scalar_style)) {
                goto done;
            }
            break;
        }

        case 1: {
            // Sequence with up to 2 scalars
            yaml_sequence_style_t seq_style =
                ((selector >> 4) & 1) ? YAML_FLOW_SEQUENCE_STYLE
                                      : YAML_BLOCK_SEQUENCE_STYLE;

            if (!yaml_sequence_start_event_initialize(&event, nullptr, nullptr, 1, seq_style) ||
                !EmitOwned(&emitter, &event)) {
                goto done;
            }

            size_t mid = payload_size / 2;

            if (!EmitScalar(&emitter, payload, mid, scalar_style)) {
                goto done;
            }
            if (!EmitScalar(&emitter, payload + mid, payload_size - mid, scalar_style)) {
                goto done;
            }

            if (!yaml_sequence_end_event_initialize(&event) ||
                !EmitOwned(&emitter, &event)) {
                goto done;
            }
            break;
        }

        case 2: {
            // Mapping with one key/value pair
            yaml_mapping_style_t map_style =
                ((selector >> 4) & 1) ? YAML_FLOW_MAPPING_STYLE
                                      : YAML_BLOCK_MAPPING_STYLE;

            if (!yaml_mapping_start_event_initialize(&event, nullptr, nullptr, 1, map_style) ||
                !EmitOwned(&emitter, &event)) {
                goto done;
            }

            size_t mid = payload_size / 2;

            if (!EmitScalar(&emitter, payload, mid, YAML_PLAIN_SCALAR_STYLE)) {
                goto done;
            }
            if (!EmitScalar(&emitter, payload + mid, payload_size - mid, scalar_style)) {
                goto done;
            }

            if (!yaml_mapping_end_event_initialize(&event) ||
                !EmitOwned(&emitter, &event)) {
                goto done;
            }
            break;
        }
    }

    // 3. Document end
    if (!yaml_document_end_event_initialize(&event, 1) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

    // 4. Stream end
    if (!yaml_stream_end_event_initialize(&event) ||
        !EmitOwned(&emitter, &event)) {
        goto done;
    }

done:
    yaml_emitter_delete(&emitter);
    return 0;
}
