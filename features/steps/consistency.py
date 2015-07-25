import hashlib
import os.path
import subprocess
import os
import time


def get_dist_name():
    found_executable = os.getenv('DIST_LOC', 'dist_location_not_set')
    print(found_executable)  # print statements are only executed by behave
    # when the behavior fails, so this print is usually silenced if all is well
    return found_executable


def run_conversion(args):
    print(args)
    running_proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)  # suppressing output
    while running_proc.poll() is None:
        time.sleep(0.1)
    assert running_proc.returncode is 0


@given(u'an input image {image} without a {converted_image}')
def step_impl(context, image, converted_image):
    assert os.path.isfile(image)
    if os.path.isfile(converted_image):
        os.chmod(converted_image, 0666)
        os.remove(converted_image)


@when(u'the {image} is converted by the code to {file_type}')
def step_impl(context, image, file_type):
    found_executable = get_dist_name()
    file_flag = ''.join(('-', file_type.lower()))  # being lazy here, letting file type match the switch
    args = [found_executable, file_flag, "-no-denoise", "-color", "none", image]
    run_conversion(args)


@when(u'the {image} is converted and compressed by the code to {file_type}')
def step_impl(context, image, file_type):
    found_executable = get_dist_name()
    file_flag = ''.join(('-', file_type.lower()))  # being lazy here, letting file type match the switch
    args = [found_executable, file_flag, "-no-denoise", "-color", "none", "-compress", image]
    run_conversion(args)


@when(u'the {image} is denoised and converted by the code')
def step_impl(context, image):
    found_executable = get_dist_name()
    args = [found_executable, '-dng', image]
    run_conversion(args)


@when(u'the {image} is denoised and converted by the code to a cropped color TIFF')
def step_impl(context, image):
    found_executable = get_dist_name()
    args = [found_executable, '-tiff', '-color', 'AdobeRGB', '-crop', image]
    run_conversion(args)


@when(u'the {image} is converted to tiff {output_format}')
def step_impl(context, image, output_format):
    found_executable = get_dist_name()
    output_switch = '-crop'
    if output_format == 'UNPROCESSED':
        output_switch = '-unprocessed'
    if output_format == 'QTOP':
        output_switch = '-qtop'
    if output_format == 'COLOR_SRGB':
        output_switch = '-color sRGB'
    if output_format == 'COLOR_ADOBE_RGB':
        output_switch = '-color AdobeRGB'
    if output_format == 'COLOR_PROPHOTO_RGB':
        output_switch = '-color ProPhotoRGB'
    output_split = output_switch.split(' ')
    # there's probably a more pythonic way to do this, with array flattening etc.
    # just being expedient for the time being
    args = [found_executable, '-tiff', '-color', 'none', '-no-denoise', output_split[0], image]
    if len(output_split) == 2:
        args = [found_executable, '-tiff', '-color', 'none', '-no-denoise', output_split[0], output_split[1], image]
    run_conversion(args)

@then(u'the {converted_image} has the right {md5} hash value')
def step_impl(context, converted_image, md5):
    assert os.path.isfile(converted_image)
    with open(converted_image) as ci:
        found_hash = hashlib.md5(ci.read()).hexdigest()
        print("found_hash: ", found_hash, " expected_hash: ", md5)
        assert md5 == found_hash
    os.chmod(converted_image, 0666)
    os.remove(converted_image)  # normally, I'd remove this file in the environment
    # however, if these files should always be removed, then remove them immediately after
    # the test should be sufficient.  This should be the last 'then' statement
    # if more tests are later made.
