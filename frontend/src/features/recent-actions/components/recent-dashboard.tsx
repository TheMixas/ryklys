import {Card} from "@radix-ui/themes";

const RecentDashboard = () => {
    return (
        <Card style={{height:'50vh'}}>
            <h1 className="text-2xl font-bold">Recent Actions</h1>
            <p className="text-gray-600">Here you can see all your recent actions and activities.</p>
        </Card>
    );
}

export default RecentDashboard;