import base64
import mimetypes
import requests


def get_content_type(filename):
    return mimetypes.guess_type(filename)[0] or 'application/octet-stream'


def get(url, headers):
    return requests.get(url, headers=headers)


def post(url, params, headers):
    return requests.post(url, json=params, headers=headers)


def post_with_file(url, files, params, headers):
    return requests.post(url, files=files, data=params, headers=headers)
