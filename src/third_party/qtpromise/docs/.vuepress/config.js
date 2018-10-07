module.exports = {
    title: 'QtPromise',
    description: 'Promises/A+ implementation for Qt/C++',
    ga: 'UA-113899811-1',
    head: [
        ['link', { rel: 'icon', href: `/favicon.png` }],
    ],
    themeConfig: {
        repo: 'simonbrunel/qtpromise',
        lastUpdated: 'Last Updated',
        editLinks: true,
        docsDir: 'docs',
        algolia: {
            apiKey: '0e6e9cccb8c2c360a5543e28c4e31cb8',
            indexName: 'qtpromise'
        },
        sidebar: [
            'qtpromise/getting-started',
            'qtpromise/qtconcurrent',
            'qtpromise/thread-safety',
            'qtpromise/api-reference',
            {
                title: 'QPromise',
                children: [
                    'qtpromise/qpromise/constructor',
                    'qtpromise/qpromise/delay',
                    'qtpromise/qpromise/each',
                    'qtpromise/qpromise/fail',
                    'qtpromise/qpromise/filter',
                    'qtpromise/qpromise/finally',
                    'qtpromise/qpromise/isfulfilled',
                    'qtpromise/qpromise/ispending',
                    'qtpromise/qpromise/isrejected',
                    'qtpromise/qpromise/map',
                    'qtpromise/qpromise/tap',
                    'qtpromise/qpromise/tapfail',
                    'qtpromise/qpromise/then',
                    'qtpromise/qpromise/timeout',
                    'qtpromise/qpromise/wait',
                    'qtpromise/qpromise/all.md',
                    'qtpromise/qpromise/reject.md',
                    'qtpromise/qpromise/resolve.md'
                ]
            },
            {
                title: 'Helpers',
                children: [
                    'qtpromise/helpers/attempt',
                    'qtpromise/helpers/each',
                    'qtpromise/helpers/filter',
                    'qtpromise/helpers/map',
                    'qtpromise/helpers/qpromise',
                    'qtpromise/helpers/qpromiseall'
                ]
            }
        ]
    }
}
