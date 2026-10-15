#!/usr/bin/env python3
"""build_apkg.py - builds ag-interviews.apkg with the anki library that comes with Anki itself.

Run it with Anki's own Python (no pip needed) - see README.md:
  A=/usr/local/share/anki
  LD_LIBRARY_PATH=$A/python/lib PYTHONHOME=$A/python PYTHONPATH=$A/app_packages \
    $A/python/bin/python3 build_apkg.py

The cards are the ones in make_deck.py. Every note gets a GUID derived from its front, so
importing a newer .apkg updates the existing cards (review history stays).
"""

import hashlib
import os
import sys
import tempfile

from anki.collection import Collection, ExportAnkiPackageOptions
from anki.import_export_pb2 import ExportLimit  # noqa: F401 (documents the type)
from anki.collection import NoteIdsLimit

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_deck  # noqa: E402

NOTETYPE = "OS AG (basic)"
CSS = """
.card { font-family: system-ui, sans-serif; font-size: 19px; line-height: 1.45;
        text-align: left; max-width: 46em; margin: 0 auto; padding: 0.4em; }
code { font-family: ui-monospace, "DejaVu Sans Mono", monospace; font-size: 0.9em;
       background: rgba(127, 127, 127, 0.15); border-radius: 4px; padding: 0 0.25em; }
pre  { font-family: ui-monospace, "DejaVu Sans Mono", monospace; font-size: 0.85em;
       background: rgba(127, 127, 127, 0.12); border-radius: 6px; padding: 0.6em; }
.deck { font-size: 0.7em; opacity: 0.6; margin-bottom: 0.6em; }
hr#answer { margin: 0.8em 0; }
"""
FRONT = '<div class="deck">{{Deck}}</div>{{Front}}'
BACK = '{{FrontSide}}<hr id="answer">{{Back}}'


def guid_for(front):
    # stable and unique per card: a hash of the front text (Anki accepts any short string)
    return "ag" + hashlib.sha1(front.encode()).hexdigest()[:18]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "ag-interviews.apkg")
    with tempfile.TemporaryDirectory() as tmp:
        col = Collection(os.path.join(tmp, "build.anki2"))

        basic = col.models.by_name("Basic")
        notetype = col.models.copy(basic, add=False)
        notetype["name"] = NOTETYPE
        notetype["css"] = CSS
        notetype["tmpls"][0]["qfmt"] = FRONT
        notetype["tmpls"][0]["afmt"] = BACK
        col.models.add(notetype)
        notetype = col.models.by_name(NOTETYPE)

        note_ids = []
        for deck, cards in make_deck.DECKS.items():
            deck_id = col.decks.id(make_deck.ROOT + "::" + deck)
            for front, back, tags in cards:
                note = col.new_note(notetype)
                note["Front"] = front
                note["Back"] = back.replace("\n", "<br>")
                note.tags = ["ag::" + t for t in tags.split()]
                note.guid = guid_for(front)
                col.add_note(note, deck_id)
                note_ids.append(note.id)

        options = ExportAnkiPackageOptions(with_scheduling=False, with_deck_configs=False,
                                           with_media=False, legacy=False)
        count = col.export_anki_package(out_path=out, options=options, limit=NoteIdsLimit(note_ids))
        col.close()
    print(f"{count} notes -> {out}")


if __name__ == "__main__":
    main()
