/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      infhost.hpp
 * PURPOSE:   Minimal INF file parser
 */

#pragma once

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <cstring>
#include "typedefs.hpp"

#define MAX_INF_STRING_LENGTH 4096

struct InfLine {
    std::vector<std::u16string> fields;
};

struct InfSection {
    std::vector<InfLine> lines;
};

struct InfFile {
    std::map<std::u16string, InfSection> sections;
};

struct InfContext {
    InfFile* file;
    std::u16string section;
    size_t line_index;
};

using HINF = InfFile*;
using PINFCONTEXT = InfContext*;

inline std::u16string trim(const std::u16string& str) {
    size_t start = 0, end = str.length();
    while (start < end && (str[start] == u' ' || str[start] == u'\t')) start++;
    while (end > start && (str[end-1] == u' ' || str[end-1] == u'\t' || str[end-1] == u'\r' || str[end-1] == u'\n')) end--;
    return str.substr(start, end - start);
}

inline std::u16string parse_field(const std::u16string& str, size_t& pos) {
    std::u16string result;
    while (pos < str.length() && (str[pos] == u' ' || str[pos] == u'\t')) pos++;
    if (pos >= str.length()) return result;

    if (str[pos] == u'"') {
        pos++;
        while (pos < str.length() && str[pos] != u'"') {
            result += str[pos++];
        }
        if (pos < str.length()) pos++;
    } else {
        while (pos < str.length() && str[pos] != u',' && str[pos] != u';') {
            result += str[pos++];
        }
        result = trim(result);
    }

    while (pos < str.length() && (str[pos] == u' ' || str[pos] == u'\t')) pos++;
    if (pos < str.length() && str[pos] == u',') pos++;

    return result;
}

inline int InfHostOpenFile(HINF* InfHandle, const char* FileName, ULONG, ULONG*) {
    std::ifstream file(FileName);
    if (!file.is_open()) return -1;

    InfFile* inf = new InfFile();
    std::u16string current_section;
    std::string line;

    while (std::getline(file, line)) {
        std::u16string wline(line.begin(), line.end());
        wline = trim(wline);

        if (wline.empty() || wline[0] == u';') continue;

        if (wline[0] == u'[' && wline.back() == u']') {
            current_section = wline.substr(1, wline.length() - 2);
            inf->sections[current_section] = InfSection();
        } else if (!current_section.empty()) {
            InfLine inf_line;
            size_t pos = 0;
            while (pos < wline.length() && wline[pos] != u';') {
                std::u16string field = parse_field(wline, pos);
                if (!field.empty() || inf_line.fields.size() > 0) {
                    inf_line.fields.push_back(field);
                }
            }
            if (!inf_line.fields.empty()) {
                inf->sections[current_section].lines.push_back(inf_line);
            }
        }
    }

    *InfHandle = inf;
    return 0;
}

inline int InfHostFindFirstLine(HINF hInf, const WCHAR* Section, const WCHAR*, PINFCONTEXT* Context) {
    if (!hInf || !Section) return -1;

    std::u16string sec(Section);
    if (hInf->sections.find(sec) == hInf->sections.end()) return -1;
    if (hInf->sections[sec].lines.empty()) return -1;

    InfContext* ctx = new InfContext();
    ctx->file = hInf;
    ctx->section = sec;
    ctx->line_index = 0;
    *Context = ctx;
    return 0;
}

inline int InfHostFindNextLine(PINFCONTEXT Context, PINFCONTEXT OutContext) {
    if (!Context || !Context->file) return -1;

    auto& section = Context->file->sections[Context->section];
    if (Context->line_index + 1 >= section.lines.size()) return -1;

    OutContext->file = Context->file;
    OutContext->section = Context->section;
    OutContext->line_index = Context->line_index + 1;
    return 0;
}

inline int InfHostGetStringField(PINFCONTEXT Context, ULONG FieldIndex, WCHAR* Buffer, ULONG BufferSize, ULONG* RequiredSize) {
    if (!Context || !Context->file) return -1;

    auto& line = Context->file->sections[Context->section].lines[Context->line_index];
    if (FieldIndex == 0 || FieldIndex > line.fields.size()) return -1;

    const std::u16string& field = line.fields[FieldIndex - 1];
    if (RequiredSize) *RequiredSize = field.length() + 1;

    if (Buffer && BufferSize > 0) {
        size_t copy_len = (field.length() < BufferSize - 1) ? field.length() : BufferSize - 1;
        RtlCopyMemory(Buffer, field.c_str(), copy_len * sizeof(WCHAR));
        Buffer[BufferSize - 1] = 0;
    }
    return 0;
}

inline int InfHostGetIntField(PINFCONTEXT Context, ULONG FieldIndex, INT* Value) {
    if (!Context || !Context->file || !Value) return -1;

    auto& line = Context->file->sections[Context->section].lines[Context->line_index];
    if (FieldIndex == 0 || FieldIndex > line.fields.size()) return -1;

    const std::u16string& field = line.fields[FieldIndex - 1];
    // Parse integer from char16_t string (ASCII-compatible)
    {
        std::string narrow(field.begin(), field.end());
        *Value = static_cast<INT>(std::strtol(narrow.c_str(), nullptr, 0));
    }
    return 0;
}

inline int InfHostGetFieldCount(PINFCONTEXT Context) {
    if (!Context || !Context->file) return 0;
    return Context->file->sections[Context->section].lines[Context->line_index].fields.size();
}

inline int InfHostGetMultiSzField(PINFCONTEXT Context, ULONG FieldIndex, WCHAR* Buffer, ULONG BufferSize, ULONG* RequiredSize) {
    return InfHostGetStringField(Context, FieldIndex, Buffer, BufferSize, RequiredSize);
}

inline int InfHostGetBinaryField(PINFCONTEXT Context, ULONG FieldIndex, unsigned char* Buffer, ULONG BufferSize, ULONG* RequiredSize) {
    if (!Context || !Context->file) return -1;

    auto& line = Context->file->sections[Context->section].lines[Context->line_index];
    if (FieldIndex == 0 || FieldIndex > line.fields.size()) return -1;

    std::vector<unsigned char> data;
    for (size_t i = FieldIndex - 1; i < line.fields.size(); i++) {
        const std::u16string& field = line.fields[i];
        std::string narrow(field.begin(), field.end());
        unsigned long val = std::strtoul(narrow.c_str(), nullptr, 16);
        data.push_back(static_cast<unsigned char>(val));
    }

    if (RequiredSize) *RequiredSize = data.size();
    if (Buffer && BufferSize > 0) {
        size_t copy_size = (data.size() < BufferSize) ? data.size() : BufferSize;
        memcpy(Buffer, data.data(), copy_size);
    }
    return 0;
}

inline void InfHostFreeContext(PINFCONTEXT Context) {
    delete Context;
}

inline void InfHostCloseFile(HINF hInf) {
    delete hInf;
}
