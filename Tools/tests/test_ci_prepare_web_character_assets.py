"""Offline regression for transient pinned model asset download retries."""
import unittest
import urllib.error
from unittest.mock import patch

from Tools import prepare_web_character_assets as assets


class FakeResponse:
    def __init__(self, payload):
        self.payload = payload

    def __enter__(self):
        return self

    def __exit__(self, *_):
        return False

    def read(self, limit):
        return self.payload[:limit]


class TestPinnedDownload(unittest.TestCase):
    def test_timeout_recovers_with_exact_download(self):
        payload = b'pinned-asset-bytes'
        with patch.object(assets.urllib.request, 'urlopen',
                          side_effect=[TimeoutError('temporary'), FakeResponse(payload)]) as fetch, \
             patch.object(assets.time, 'sleep') as sleep:
            self.assertEqual(assets.download('https://example.invalid/asset'), payload)
        self.assertEqual(fetch.call_count, 2)
        sleep.assert_called_once_with(1)

    def test_transient_http_503_retries(self):
        failure = urllib.error.HTTPError('https://example.invalid', 503,
                                         'Unavailable', {}, None)
        with patch.object(assets.urllib.request, 'urlopen',
                          side_effect=[failure, FakeResponse(b'fine')]) as fetch, \
             patch.object(assets.time, 'sleep') as sleep:
            self.assertEqual(assets.download('https://example.invalid'), b'fine')
        self.assertEqual(fetch.call_count, 2)
        sleep.assert_called_once_with(1)

    def test_404_is_not_retried(self):
        failure = urllib.error.HTTPError('https://example.invalid', 404,
                                         'Not Found', {}, None)
        with patch.object(assets.urllib.request, 'urlopen', side_effect=failure) as fetch, \
             patch.object(assets.time, 'sleep') as sleep:
            with self.assertRaises(urllib.error.HTTPError):
                assets.download('https://example.invalid')
        fetch.assert_called_once()
        sleep.assert_not_called()

    def test_persistent_timeout_stops_and_reports_error(self):
        with patch.object(assets.urllib.request, 'urlopen',
                          side_effect=TimeoutError('no network')) as fetch, \
             patch.object(assets.time, 'sleep') as sleep:
            with self.assertRaises(TimeoutError):
                assets.download('https://example.invalid')
        self.assertEqual(fetch.call_count, 3)
        self.assertEqual([call.args[0] for call in sleep.call_args_list], [1, 2])

    def test_oversized_bytes_are_rejected_without_retry(self):
        huge = FakeResponse(b'x' * (20 * 1024 * 1024 + 1))
        with patch.object(assets.urllib.request, 'urlopen', return_value=huge) as fetch, \
             patch.object(assets.time, 'sleep') as sleep:
            with self.assertRaisesRegex(ValueError, 'download budget'):
                assets.download('https://example.invalid')
        fetch.assert_called_once()
        sleep.assert_not_called()


if __name__ == '__main__':
    unittest.main()
