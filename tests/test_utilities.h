#pragma once

#include "core/io/file_access_pack.h"
#include "modules/gdscript/gdscript_analyzer.h"
#include "modules/gdscript/gdscript_compiler.h"
#include "modules/gdscript/gdscript_parser.h"

bool build_and_assign_script(Object *p_object, const String &p_code) {
	ScriptServer::init_languages();

	Ref<GDScript> script;

	script.instance();
	GDScriptParser parser;
	Error err = parser.parse(p_code, "res://test/justs/a/test/path/script.gd", false);
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

	script->set_path("res://test/justs/a/test/path/script.gd");

	GDScriptCompiler compiler;
	err = compiler.compile(&parser, script.ptr(), false);
	ERR_FAIL_COND_V(err != OK, false);
	p_object->set_script(script);

	// Destroy stuff we set up earlier.
	ScriptServer::finish_languages();

	return true;
}
