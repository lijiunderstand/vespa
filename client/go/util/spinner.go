// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

package util

import (
	"fmt"
	"os"
	"time"

	"github.com/briandowns/spinner"
	"github.com/pkg/errors"
)

const (
	spinnerTextDone   = "done"
	spinnerTextFailed = "failed"
	spinnerColor      = "blue"
)

var messages = os.Stderr

func Spinner(text string, fn func() error) {
	initialMsg := text + " "
	doneMsg := "\r" + initialMsg + spinnerTextDone + "\n"
	failMsg := "\r" + initialMsg + spinnerTextFailed + "\n"
	loading(initialMsg, doneMsg, failMsg, fn)
}

func Waiting(fn func() error) {
	loading("", "", "", fn)
}

func loading(initialMsg, doneMsg, failMsg string, fn func() error) {
	done := make(chan struct{})
	errc := make(chan error)
	go func() {
		defer close(done)

		s := spinner.New(spinner.CharSets[11], 100*time.Millisecond, spinner.WithWriter(messages))
		s.Prefix = initialMsg
		s.FinalMSG = doneMsg
		s.HideCursor = true
		s.Writer = messages

		if err := s.Color(spinnerColor, "bold"); err != nil {
			panic(Error(err, "failed setting spinner color"))
		}

		s.Start()
		err := <-errc
		if err != nil {
			s.FinalMSG = failMsg
		}

		s.Stop()
	}()

	err := fn()
	errc <- err
	<-done

	if err != nil {
		fmt.Println(fmt.Errorf("an unexpected error occurred: %w", err))
	}
}

func Error(e error, message string) error {
	return errors.Wrap(e, message)
}