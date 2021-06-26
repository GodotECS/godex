#pragma once

#include "../modules/godot/nodes/script_ecs.h"
#include "core/io/file_access_pack.h"
#include "modules/gdscript/gdscript_analyzer.h"
#include "modules/gdscript/gdscript_compiler.h"
#include "modules/gdscript/gdscript_parser.h"

bool script_initialized = false;

void initialize_script_ecs() {
	ScriptServer::init_languages();
	script_initialized = true;
}

void finalize_script_ecs() {
	ScriptEcs::get_singleton()->__empty_scripts();
	ScriptServer::finish_languages();
	script_initialized = false;
}

bool build_and_register_ecs_script(const StringName &p_script_name, const String &p_code) {
	CRASH_COND_MSG(script_initialized == false, "Please call `initialize_script_ecs` before executing the function. So call the function `finalize_script_ecs` at the end.");

	Ref<GDScript> script;
	script.instantiate();
	GDScriptParser parser;
	Error err = parser.parse(p_code, "res://test/justs/a/test/path/" + p_script_name, false);
	ERR_FAIL_COND_V(err != OK, false);

	GDScriptAnalyzer analyzer(&parser);
	err = analyzer.analyze();
	if (err != OK) {
		print_line("Error in analyzer:");
		const List<GDScriptParser::ParserError> &errors = parser.get_errors();
		for (const List<GDScriptParser::ParserError>::Element *E = errors.front(); E != nullptr; E = E->next()) {
			const GDScriptParser::ParserError &error = E->get();
			print_line(vformat("%02d:%02d: %s", error.line, error.column, error.message));
		}
		return false;
	}

	script->set_path("res://test/justs/a/test/path/" + p_script_name);

	GDScriptCompiler compiler;
	err = compiler.compile(&parser, script.ptr(), false);
	ERR_FAIL_COND_V(err != OK, false);

	return ScriptEcs::get_singleton()->__reload_script(script, "res://test/justs/a/test/path/" + p_script_name, p_script_name);
}

void flush_ecs_script_preparation() {
	ScriptEcs::get_singleton()->flush_scripts_preparation();
}
