def can_build(env, platform):
    return True


def configure(env):
    pass


def has_custom_iterator():
    return True


def has_custom_physics_iterator():
    return True


def has_custom_audio_iterator():
    # TODO enable custom iterator once the audio process system is integrated
    return False


def get_doc_path():
    return "doc_classes"


def get_doc_classes():
    return [
        "Component",
        "DynamicQuery",
        "ECS",
        # Disabled until only 'Entity' exists. 'doctool' will generate in 'godot/docs/classes' instead.
        # "Entity2D",
        # "Entity3D",
        "PipelineECS",
        "System",
        "WorldECS",
    ]
