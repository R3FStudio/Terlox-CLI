# Try the examples

Run these from the project folder inside Terlox. They need Python's standard
library, with no extra packages.

| Command | What you'll see |
| --- | --- |
| `python examples/showcase.py` | The CLI example used in the README screenshot |
| `python examples/rgb_demo.py` | RGB text and background colors |
| `python examples/graphics_demo.py` | An inline PNG graphic |
| `python examples/theme_demo.py` | Temporary colors, opacity and text scaling |

The theme example waits for Enter, then restores the previous appearance.
`protocol.py` contains reusable helpers for those controls and inline images.
For the supported formats and limits, see
[CLI application support](../docs/reference/cli-protocols.md).
