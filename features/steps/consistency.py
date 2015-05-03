@given(u'an input image {image}')
def step_impl(context):
    raise NotImplementedError(u'STEP: Given an input image')


@when(u'the {image} is converted by the code to {file_type}')
def step_impl(context):
    raise NotImplementedError(u'STEP: the {image} is converted by the code to {file_type}')


@then(u'the {converted_image} has the right {md5} hash value')
def step_impl(context):
    raise NotImplementedError(u'the {converted_image} has the right {md5} hash value')
