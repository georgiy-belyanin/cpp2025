with import <nixpkgs> {};
pkgs.mkShell {
    name = "cpp2025";
    packages = [ 
      # Lsp, treesitter-parsers and debugger
      pkgs.rust-analyzer
      pkgs.rustfmt
      pkgs.vimPlugins.nvim-treesitter-parsers.yaml
      pkgs.vimPlugins.nvim-treesitter-parsers.jsonc
      pkgs.vimPlugins.nvim-treesitter-parsers.markdown
      pkgs.vimPlugins.nvim-treesitter-parsers.markdown_inline
      pkgs.vimPlugins.nvim-treesitter-parsers.nix

      # Build tools
      pkgs.rustc
      pkgs.cargo
    ];
    shellHook = '''';
}
