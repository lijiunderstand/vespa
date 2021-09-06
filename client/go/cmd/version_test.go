package cmd

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestVersion(t *testing.T) {
	assert.Equal(t, "vespa version 0.0.0-devel\n", execute(command{args: []string{"version"}}, t, nil))
}