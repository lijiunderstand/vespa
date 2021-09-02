// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Root Cobra command: vespa
// author: bratseth

package cmd

import (
	"log"
	"os"

	"github.com/logrusorgru/aurora"
	"github.com/mattn/go-colorable"
	"github.com/mattn/go-isatty"
	"github.com/spf13/cobra"
)

var (
	// TODO: add timeout flag
	// TODO: add flag to show http request made
	rootCmd = &cobra.Command{
		Use:   "vespa <command>",
		Short: "The command-line tool for Vespa.ai",
		Long: `The command-line tool for Vespa.ai.

Use it on Vespa instances running locally, remotely or in the cloud.
Prefer web service API's to this in production.

Vespa documentation: https://docs.vespa.ai`,
	}
	color          aurora.Aurora
	targetArg      string
	applicationArg string
)

const (
	applicationFlag = "application"
	targetFlag      = "target"
)

func configureLogger() {
	color = aurora.NewAurora(isatty.IsTerminal(os.Stdout.Fd()))
	log.SetFlags(0) // No timestamps
	log.SetOutput(colorable.NewColorableStdout())
}

func init() {
	configureLogger()
	cobra.OnInitialize(readConfig)
	rootCmd.PersistentFlags().StringVarP(&targetArg, targetFlag, "t", "local", "The name or URL of the recipient of this command")
	rootCmd.PersistentFlags().StringVarP(&applicationArg, applicationFlag, "a", "", "The application to manage")
	bindFlagToConfig(targetFlag, rootCmd)
	bindFlagToConfig(applicationFlag, rootCmd)
}

// Execute executes the root command.
func Execute() error { return rootCmd.Execute() }