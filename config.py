def can_build(env, platform):
    return True


def configure(env):
    pass


def has_custom_iterator():
    return True


def has_custom_physics_iterator():
    # TODO enable custom iterator once the physics process system is integrated
    return False


def has_custom_audio_iterator():
    # TODO enable custom iterator once the audio process system is integrated
    return False
