import { useState } from 'react';
import { Flex, Grid, Button, Text } from '@radix-ui/themes';

const StreamQuickActionGrid = () => {
    const [actions, setActions] = useState<string[]>(['Action 1', 'Action 2', 'Action 3', 'Action 4']);
    const maxActions = 14; // 3 rows * 5 items - 1 (for add button) = 14

    const handleAddAction = () => {
        if (actions.length < maxActions) {
            setActions([...actions, `Action ${actions.length + 1}`]);
        }
    };

    return (
        <Flex direction="column" gap="3">
            <Text size="5" weight="bold">Stream quick actions</Text>
            <Grid columns="5" gap="3">
                {actions.map((action, index) => (
                    <Button key={index} variant="soft">
                        {action}
                    </Button>
                ))}
                {actions.length < maxActions && (
                    <Button onClick={handleAddAction} variant="outline">
                        + Add Action
                    </Button>
                )}
            </Grid>
        </Flex>
    );
};

export default StreamQuickActionGrid;