"""Reverse only atomic-policy save scopes; keep preceding full-source pins."""
import base64
import zlib

OLD = {
    "remove": "eJxtkcFOwzAMhu97CrNTOw32ANVAhYKExAk4TorSxaERS9Il0cRAvDtOQ7dl4Fvs//8/ywHIazGDZ9R2hzBbTFJrchgqCYVQnrcbFEwq5wMsl9CX0WWsQAgWWgQ3BIg465ALuDlmpTrPgP7y2uBHqI4q3HiMsQ/KCAgdpvwWpXWE6ZQHazDL/coQpINiS9E5q4JtQsEFUelFit9OWUUe6j7sB/fZzqPvv2W//1xKOkTmg1PmDQoyHLZo94kTBWC4RrBy3DFnDhEaNfnnXn2ilUXz+FLfPt03rKlf6/I0KB7n1Oz5DtkILZIy9uIPUfs9F6MRLFi27jgdbXpnteZ0dTSD+2plVm46X3dlNfkBjUGWYQ==",
    "add": "eJxdkE1OwzAQhfc9xagL5FS0PUBEpbRhgcSu7CiyHHvSRPgnGjsVcHrsNg0J3s3M+948D8DwOngCobWT3KAB5tsfdDUrX47F/vW55GXxVmRZvriJFzBi6510xgirIv8gjeJBVBrf2498JlKtT33Fq+8o9IG46jtgslnvrDCY5bBdgRcXhFSCq2EgCFbbmZPGC+roccbAA/U+RJMJfm39Zyx+hYiMIeqWfJgEnA+isrsaCqWgwtpRtG0QZE+ENsBNhBpNqsZVf3YebbyD47IRBGx5GA5037I52RMtH1PsCRPD87uCTT/kEviZ9vwCvRR7rQ==",
    "save_disabled": "eJyNUV1rwjAUffdX3BWEtCjueeKgWzuQuT3oHgelmpuSLU1Ckupk+N+X+FGte/FCm+Zw7rnnno4SWJRrBMptuRRIYaXqupTUQjLqAcBacQrWM4oTg8QB/w0vXy/TWQ4J0+PjPZsu0qdZnhVZ+pFC0uKckbuS1ly2OoXBkm5JHJ+kfAlVFdYZLisSdYY+gMEvXLm9Qcl41ZjScSWBWwg6QyXFNorHZymDrjHyBOzC2VoBcte6YNxYF8MoAYoCHQLjAgNHqnMWpblI6BDMRQa+Gim4/AbSrh9yuc0O0zABpjTKq/YBRJvjSpfOmY7/z182FZDoWTWCeuMO9nIRdPT8nSkDG8OdDzga3N/oz/eQ4LEb2Rg0hGcCevgo8cfFZzGm/S903izTfom+hb7g0LefMhoE9jFW31bWuEcErlHsv9ohy21n9WvJoJW/Z8VbOn/N513qSiiLgXmAd73eHwVIyCA=",
    "save_bans": "eJyNkF1LwzAUhu/3K94VhGQ48doyoaKCMHald0Lp2qREs5OQpHNj7L+b7KOrsAtzkS/OefK8AbA2qoGv1qJcVuRZOvIRgF2a4nh9m79gIm1+Oj8Vi/K5eC8w6a+UZOOqWSk6IEonqmbLOD8T4tCmLX1wilqW9W89wIkvUQfRoDYkVdu5KihDUB6JMTWktxnPLxgnQufofLFPa68ANo7QUisf+Kl8INCRVvQNlsWau7AJ/8RKpUXM44WLzrU2XrC+UVrMII0VNMDeIvs5sYdm0nLsll1MX5tONyATcOjs++6vCP0xMQ4svXgOmcNiPMPiYz5P2xns9JHEJoYfxJY2/nmQTNooduM/KQqmumol+BGfio7BomN+LXbyZJdI+9EvgSOXIA==",
    "ban_add": "eJxTUFBQKEhKzNO1y0vMTeW0VSguKYpPKS3QUEgsSlfQtOZSQChIrSgBKgCy43Myi0sgUjAeUAKkCiJYnJqXEl+SH5+ckVikoaDkn60XkxdTpKSjkJwBM7I4sSw1HqihWAMkAgBpwScB",
    "ban_remove": "eJxFjTEOwjAMRWf3FFanVAIugMIJKpjYKkUhuLSiBGSHiuOTKAl48/d734Bx5hEVvphW1BqP577HrgG4WG+WWUIENNZle/D0CfsGkkeLUASTmfMIujdzhTI1MpGRwLO/qXq2D8KutPwNyH+SUG51hVxcUiF/NeFp3GRZYXu67wY/cLtBN/1axa5koi4qRV+tWkHE",
}

def restore(text, start, end, name):
    assert text.count(start) == text.count(end) == 1, (start, end)
    a = text.index(start)
    b = text.index(end, a) + len(end)
    return text[:a] + zlib.decompress(base64.b64decode(OLD[name])).decode() + text[b:]

def without_admin_saves_stage(text):
    from aux_shutdown_source import without_aux_shutdown_stage
    text=without_aux_shutdown_stage(text)
    if "/* Atomic disabled-command removal. */" in text:
        text = restore(text, "                /* Atomic disabled-command removal. */\n",
                       "                /* End atomic disabled-command removal. */\n", "remove")
        text = restore(text, "                /* Atomic disabled-command addition. */\n",
                       "                /* End atomic disabled-command addition. */\n", "add")
        a = text.index("/* Save disabled commands */")
        b = text.index("   /* Ban reader lives in admin_config.c. */", a)
        text = text[:a] + zlib.decompress(base64.b64decode(OLD["save_disabled"])).decode() + text[b:]
        a = text.index("   void save_bans(void)")
        text = text[:a] + zlib.decompress(base64.b64decode(OLD["save_bans"])).decode()
    elif "/* Atomic ban addition. */" in text:
        text = restore(text, "    /* Atomic ban addition. */\n",
                       "    /* End atomic ban addition. */\n", "ban_add")
        text = restore(text, "\t    /* Atomic ban removal. */\n",
                       "\t    /* End atomic ban removal. */\n", "ban_remove")
    elif "/* Atomic policy save implementations." in text:
        text = text[:text.index("\n/* Atomic policy save implementations.")]
        text = text.replace('#include "file_io.h"\n', "")
    elif "/* Administrative policy fixed-name atomic APIs. */" in text:
        text = text[:text.index("\n/* Administrative policy fixed-name atomic APIs. */")]
    elif "#define ATOMIC_SAVE_H\n" in text:
        a = text.index("/* Fixed administrative filenames only;")
        b = text.index("/* Name is an ASCII alphabetic player name", a)
        text = text[:a] + "\n" + text[b:]
        text = text.replace("SAVE_FAULT_CLEANUP, SAVE_FAULT_REMOVE", "SAVE_FAULT_CLEANUP")
    elif "#define ANATHEMA_ADMIN_CONFIG_H\n" in text:
        text = text.replace('#include "atomic_save.h"\n', "")
        for name in ("bans", "disabled"):
            text = text.replace(f"enum atomic_save_result admin_save_{name}(void);\n", "")
    return text
