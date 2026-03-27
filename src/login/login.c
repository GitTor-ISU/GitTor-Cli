#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include "login/login.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#endif

extern authentication_dto_t* parse_authentication_json(const char* json) {
    JsonParser* parser = json_parser_new();

    // Load JSON data into parser
    if (!json_parser_load_from_data(parser, json, -1, NULL)) {
        g_object_unref(parser);
        return NULL;
    }

    // Get the root to make sure it's an object
    JsonNode* root_node = json_parser_get_root(parser);
    if (!root_node || !JSON_NODE_HOLDS_OBJECT(root_node)) {
        g_object_unref(parser);
        return NULL;
    }

    // Parse JSON object into authentication_dto_t
    JsonObject* root_obj = json_node_get_object(root_node);
    authentication_dto_t* dto = g_malloc0(sizeof(authentication_dto_t));

    if (json_object_has_member(root_obj, "accessToken"))
        dto->access_token =
            g_strdup(json_object_get_string_member(root_obj, "accessToken"));

    if (json_object_has_member(root_obj, "tokenType"))
        dto->token_type =
            g_strdup(json_object_get_string_member(root_obj, "tokenType"));

    if (json_object_has_member(root_obj, "expires"))
        dto->expires =
            g_strdup(json_object_get_string_member(root_obj, "expires"));

    g_object_unref(parser);
    return dto;
}

extern char* build_login_json(const login_dto_t* dto) {
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    if (dto->email) {
        json_builder_set_member_name(builder, "email");
        json_builder_add_string_value(builder, dto->email);
    }

    if (dto->username) {
        json_builder_set_member_name(builder, "username");
        json_builder_add_string_value(builder, dto->username);
    }

    if (dto->password) {
        json_builder_set_member_name(builder, "password");
        json_builder_add_string_value(builder, dto->password);
    }

    json_builder_end_object(builder);

    JsonGenerator* generator = json_generator_new();
    JsonNode* root = json_builder_get_root(builder);
    json_generator_set_root(generator, root);
    json_node_free(root);

    gsize len = 0;
    char* json_str = json_generator_to_data(generator, &len);

    g_object_unref(generator);
    g_object_unref(builder);

    return json_str;
}

extern void authentication_dto_free(authentication_dto_t* dto) {
    if (!dto)
        return;

    g_free(dto->access_token);
    g_free(dto->token_type);
    g_free(dto->expires);
    g_free(dto);
}

extern void login_dto_free(login_dto_t* dto) {
    if (dto->email)
        g_free(dto->email);
    if (dto->username)
        g_free(dto->username);
    if (dto->password)
        g_free(dto->password);
}

extern int prompt_line(const char* prompt,
                       bool hide_input,
                       char* buffer,
                       size_t buffer_size) {
    if (!prompt || !buffer || buffer_size == 0) {
        return EINVAL;
    }

    printf("%s: ", prompt);
    fflush(stdout);

#ifdef _WIN32  // Windows implementation using _getch()
    if (hide_input) {
        size_t pos = 0;
        while (1) {
            int ch = _getch();
            if (ch == '\r' || ch == '\n') {
                // End input on Enter
                break;
            } else if ((ch == '\b' || ch == 127) && pos > 0) {
                // Handle backspace
                pos--;
            } else if (ch == 3) {
                // Handle Ctrl+C
                printf("\n");
                return EINTR;
            } else if (ch >= 32 && ch < 127 && pos < buffer_size - 1) {
                // Handle regular character
                buffer[pos++] = (char)ch;
            }
        }
        buffer[pos] = '\0';
        printf("\n");
    } else {
        if (!fgets(buffer, (int)buffer_size, stdin)) {
            return EIO;
        }
    }
#else  // Unix-like implementation using termios
    struct termios oldt;
    struct termios newt;
    bool changed_term = false;

    if (hide_input && tcgetattr(STDIN_FILENO, &oldt) == 0) {
        newt = oldt;
        newt.c_lflag &= (tcflag_t)~ECHO;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == 0) {
            changed_term = true;
        }
    }

    if (!fgets(buffer, (int)buffer_size, stdin)) {
        if (changed_term) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            printf("\n");
        }

        return EIO;
    }

    if (changed_term) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        printf("\n");
    }
#endif

    // Remove trailing newline if present
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    if (strlen(buffer) == 0) {
        return EINVAL;
    }

    return 0;
}

extern void secure_zero(void* ptr, size_t len) {
    if (ptr && len > 0) {
        volatile char* p = (volatile char*)ptr;
        while (len--) {
            *p++ = 0;
        }
    }
}
