#pragma once

#include "../modules/godot/nodes/script_ecs.h"
#include "core/io/file_access_pack.h"
#include "modules/gdscript/gdscript_analyzer.h"
#include "modules/gdscript/gdscript_compiler.h"
#include "modules/gdscript/gdscript_parser.h"

bool script_initialized = false;
HashMap<String, String> scripts;

void initialize_script_ecs() {
	ScriptServer::init_languages();
	script_initialized = true;
}

void finalize_script_ecs() {
	ScriptEcs::get_singleton()->__empty_scripts();
	ScriptServer::finish_languages();
	script_initialized = false;
	scripts.clear();
}

bool register_ecs_script(const StringName &p_script_name, const String &p_code) {
	CRASH_COND_MSG(script_initialized == false, "Please call `initialize_script_ecs` before executing the function. So call the function `finalize_script_ecs` at the end.");

	GDScriptParser parser;
	Error err = parser.parse(p_code, "res://test/justs/a/test/path/" + p_script_name, false);
	ERR_FAIL_COND_V(err != OK, false);

	for (auto extends : parser.get_tree()->extends) {
		if (extends == "System") {
			ECS::register_dynamic_system(p_script_name);
		}
	}

	scripts[p_script_name] = p_code;
	return true;
}

bool build_scripts() {
	for (auto item : scripts) {
		auto script_name = item.key;
		auto code = item.value;

		Ref<GDScript> script;
		script.instantiate();

		GDScriptParser parser;
		parser.parse(code, "res://test/justs/a/test/path/" + script_name, false);

		GDScriptAnalyzer analyzer(&parser);
		Error err = analyzer.analyze();
		if (err != OK) {
			print_line("Error in analyzer:");
			const List<GDScriptParser::ParserError> &errors = parser.get_errors();
			for (const List<GDScriptParser::ParserError>::Element *E = errors.front(); E != nullptr; E = E->next()) {
				const GDScriptParser::ParserError &error = E->get();
				print_line(vformat("%02d:%02d: %s", error.line, error.column, error.message));
			}
			return false;
		}

		script->set_path("res://test/justs/a/test/path/" + script_name);

		GDScriptCompiler compiler;
		err = compiler.compile(&parser, script.ptr(), false);
		ERR_FAIL_COND_V(err != OK, false);

		if (!ScriptEcs::get_singleton()->__reload_script(script, "res://test/justs/a/test/path/" + script_name, script_name)) {
			return false;
		}
	}
	return true;
}

void flush_ecs_script_preparation() {
	return ScriptEcs::get_singleton()->flush_scripts_preparation();
}
