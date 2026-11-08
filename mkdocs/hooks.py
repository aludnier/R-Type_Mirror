"""MkDocs hooks of the documentation site (see mkdocs/mkdocs.yml)."""

import os
import re

# Markdown links and images: [text](target) or ![alt](target "title").
LINK = re.compile(r"(!?\[[^\]]*\]\()([^)\s]+)((?:\s+\"[^\"]*\")?\))")
BRANCH = "main"


def on_page_markdown(markdown, page, config, files):
    """Links to repository files outside docs/ point to GitHub on the site.

    On GitHub, `../config/ui/example.json` works as a relative link. The site
    only contains docs/, so these links are turned into GitHub links.
    """
    docs_dir = os.path.realpath(config["docs_dir"])
    repo_root = os.path.dirname(docs_dir)
    page_dir = os.path.dirname(page.file.abs_src_path)
    repo_url = config["repo_url"].rstrip("/")

    def rewrite(match):
        target = match.group(2)
        if re.match(r"^[a-z][a-z0-9+.-]*:|^#|^/", target):
            return match.group(0)
        path, _, anchor = target.partition("#")
        absolute = os.path.realpath(os.path.join(page_dir, path))
        if absolute.startswith(docs_dir + os.sep) or not os.path.exists(absolute):
            return match.group(0)
        kind = "tree" if os.path.isdir(absolute) else "blob"
        url = f"{repo_url}/{kind}/{BRANCH}/{os.path.relpath(absolute, repo_root)}"
        if anchor:
            url += "#" + anchor
        return match.group(1) + url + match.group(3)

    return LINK.sub(rewrite, markdown)
